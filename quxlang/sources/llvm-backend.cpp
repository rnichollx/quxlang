// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#include <quxlang/llvm-backend.hpp>

#include <quxlang/exception.hpp>
#include <quxlang/bytemath.hpp>
#include <quxlang/backends/asm/arm_asm_converter.hpp>
#include <quxlang/backends/asm/x64_asm_converter.hpp>
#include <quxlang/manipulators/llvm_lookup.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/assembler.hpp>
#include <quxlang/vmir2/state_engine.hpp>

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/MCAsmBackend.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCCodeEmitter.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCObjectWriter.h>
#include <llvm/MC/MCParser/MCAsmParser.h>
#include <llvm/MC/MCParser/MCTargetAsmParser.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/LowerMemIntrinsics.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace quxlang::llvm_backend::detail
{
    using ir_builder_t = llvm::IRBuilder< llvm::ConstantFolder, llvm::IRBuilderCallbackInserter >;

    struct abi_parameter
    {
        std::optional< std::string > name;
        std::optional< std::size_t > positional_index;
        quxlang::type_symbol type;
    };

    struct routine_abi_parameter
    {
        std::optional< std::string > name;
        std::optional< std::size_t > positional_index;
        quxlang::type_symbol parameter_type;
        quxlang::vmir2::local_index local;
    };

    struct callable_abi
    {
        /// Source-level calling convention used when declaring and calling this function.
        std::string calling_convention = "DEFAULT";
        std::vector< abi_parameter > source_ordered;
        std::map< std::string, std::size_t > source_named_indices;
        std::vector< std::size_t > llvm_param_source_indices;
        std::optional< std::size_t > return_source_index;
        llvm::FunctionType* llvm_type = nullptr;
    };

    struct local_slot_state
    {
        llvm::Value* storage = nullptr;
        llvm::Value* aliased_value_address = nullptr;
    };

    struct function_codegen_state
    {
        llvm::Function* function = nullptr;
        quxlang::vmir2::functanoid_routine3 const* routine = nullptr;
        callable_abi const* abi = nullptr;
        std::vector< local_slot_state > locals;
        std::map< quxlang::vmir2::block_index, llvm::BasicBlock* > blocks;
        quxlang::vmir2::state_map current_state;
    };

    class llvm_module_codegen
    {
    public:
        explicit llvm_module_codegen(quxlang::llvm_backend::llvm_compilable_unit const& input_packet)
            : input(input_packet),
              module(std::make_unique< llvm::Module >(quxlang::to_string(input_packet.target_name), context)),
              builder(context, llvm::ConstantFolder(), llvm::IRBuilderCallbackInserter([this](llvm::Instruction* inst)
              {
                  annotate_inserted_instruction(inst);
              })),
              target_machine(create_target_machine(input_packet.machine_target.machine, input_packet.machine_target.optimization))
        {
            module->setTargetTriple(llvm::Triple(quxlang::lookup_llvm_triple(input.machine_target.machine)));
            module->setDataLayout(target_machine->createDataLayout());
            vmir2_metadata_kind = context.getMDKindID("qux.vmir2");
            if (input.source_index.has_value())
            {
                module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
                module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 5U);
                debug_builder = std::make_unique< llvm::DIBuilder >(*module);

                llvm::DIFile* const compile_file = default_debug_file();
                debug_compile_unit = debug_builder->createCompileUnit(
                    llvm::dwarf::DW_LANG_C_plus_plus,
                    compile_file,
                    "qxc",
                    input.machine_target.optimization == quxlang::llvm_backend::optimization_level::release,
                    "",
                    0);
            }
        }

        auto compile() -> quxlang::llvm_backend::llvm_compiled_unit
        {
            for (std::pair< quxlang::type_symbol const, quxlang::asm_callable > const& helper : input.asm_callable_interfaces)
            {
                declare_asm_callable_function(helper.first, helper.second);
            }
            if (!input.asm_functions.contains(input.target_name))
            {
                declare_defined_function(input.target_name, input.target_code, llvm::GlobalValue::LinkOnceODRLinkage);
            }
            for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& helper : input.inlinable_functions)
            {
                if (input.asm_functions.contains(helper.first))
                {
                    continue;
                }
                llvm::GlobalValue::LinkageTypes const helper_linkage =
                    input.whole_module ? llvm::GlobalValue::LinkOnceODRLinkage : llvm::GlobalValue::AvailableExternallyLinkage;
                declare_defined_function(helper.first, helper.second, helper_linkage);
            }

            emit_object_reference_globals();

            if (!input.asm_functions.contains(input.target_name))
            {
                emit_defined_function_with_traceback(input.target_name, input.target_code);
            }
            for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& helper : input.inlinable_functions)
            {
                if (input.asm_functions.contains(helper.first))
                {
                    continue;
                }
                emit_defined_function_with_traceback(helper.first, helper.second);
            }
            for (std::pair< quxlang::type_symbol const, quxlang::asm_procedure > const& helper : input.asm_functions)
            {
                module->appendModuleInlineAsm(assembly_text(helper.second));
            }

            if (should_emit_linux_start())
            {
                emit_linux_start();
            }

            if (debug_builder)
            {
                debug_builder->finalize();
            }

            std::string verify_error;
            llvm::raw_string_ostream verify_stream(verify_error);
            if (llvm::verifyModule(*module, &verify_stream))
            {
                throw quxlang::semantic_compilation_error("LLVM IR verification failed for " + quxlang::to_string(input.target_name) + ": " + verify_stream.str());
            }

            quxlang::llvm_backend::llvm_compiled_unit result;
            {
                llvm::raw_string_ostream ir_stream(result.llvm_ir_text);
                module->print(ir_stream, nullptr);
            }
            result.object_file = emit_module_object_file(*module, input.machine_target.optimization);
            if (input.machine_target.optimization == quxlang::llvm_backend::optimization_level::release)
            {
                std::unique_ptr< llvm::Module > optimized_module = llvm::CloneModule(*module);
                std::vector< llvm::GlobalValue* > preserved_functions;
                for (llvm::Function& function : optimized_module->functions())
                {
                    if (!function.isDeclaration())
                    {
                        preserved_functions.push_back(&function);
                    }
                }
                for (llvm::GlobalVariable& global : optimized_module->globals())
                {
                    if (!global.isDeclaration())
                    {
                        preserved_functions.push_back(&global);
                    }
                }
                llvm::appendToUsed(*optimized_module, preserved_functions);

                llvm::PassBuilder pass_builder;
                llvm::LoopAnalysisManager loop_analysis_manager;
                llvm::FunctionAnalysisManager function_analysis_manager;
                llvm::CGSCCAnalysisManager cgscc_analysis_manager;
                llvm::ModuleAnalysisManager module_analysis_manager;
                pass_builder.registerModuleAnalyses(module_analysis_manager);
                pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
                pass_builder.registerFunctionAnalyses(function_analysis_manager);
                pass_builder.registerLoopAnalyses(loop_analysis_manager);
                pass_builder.crossRegisterProxies(loop_analysis_manager, function_analysis_manager, cgscc_analysis_manager, module_analysis_manager);

                llvm::ModulePassManager module_pass_manager = pass_builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
                module_pass_manager.run(*optimized_module, module_analysis_manager);

                llvm::SmallVector< llvm::MemSetInst*, 4 > memset_intrinsics;
                for (llvm::Function& function : optimized_module->functions())
                {
                    for (llvm::BasicBlock& block : function)
                    {
                        for (llvm::Instruction& instruction : block)
                        {
                            if (auto* const memset_inst = llvm::dyn_cast< llvm::MemSetInst >(&instruction))
                            {
                                memset_intrinsics.push_back(memset_inst);
                            }
                        }
                    }
                }
                for (llvm::MemSetInst* const memset_inst : memset_intrinsics)
                {
                    llvm::expandMemSetAsLoop(memset_inst);
                    memset_inst->eraseFromParent();
                }
                llvm::SmallVector< llvm::Function*, 2 > unused_memset_declarations;
                for (llvm::Function& function : optimized_module->functions())
                {
                    if (function.isDeclaration() && function.getIntrinsicID() == llvm::Intrinsic::memset && function.use_empty())
                    {
                        unused_memset_declarations.push_back(&function);
                    }
                }
                for (llvm::Function* const unused_memset_decl : unused_memset_declarations)
                {
                    unused_memset_decl->eraseFromParent();
                }

                std::string verify_error;
                llvm::raw_string_ostream verify_stream(verify_error);
                if (llvm::verifyModule(*optimized_module, &verify_stream))
                {
                    throw quxlang::semantic_compilation_error("Optimized LLVM IR verification failed for " + quxlang::to_string(input.target_name) + ": " + verify_stream.str());
                }

                if (llvm::GlobalVariable* used = optimized_module->getGlobalVariable("llvm.used"))
                {
                    used->eraseFromParent();
                }

                llvm::raw_string_ostream ir_stream(result.optimized_llvm_ir_text);
                optimized_module->print(ir_stream, nullptr);
                result.optimized_object_file = emit_module_object_file(*optimized_module, quxlang::llvm_backend::optimization_level::release);
            }
            else
            {
                result.optimized_llvm_ir_text = result.llvm_ir_text;
                result.optimized_object_file = result.object_file;
            }
            {
                llvm::SmallVector< char, 0 > bitcode_buffer;
                llvm::raw_svector_ostream bitcode_stream(bitcode_buffer);
                llvm::WriteBitcodeToFile(*module, bitcode_stream);
                result.bitcode.resize(bitcode_buffer.size());
                for (std::size_t i = 0; i < bitcode_buffer.size(); ++i)
                {
                    result.bitcode[i] = static_cast< std::byte >(bitcode_buffer[i]);
                }
            }
            return result;
        }

    private:
        quxlang::llvm_backend::llvm_compilable_unit const& input;
        llvm::LLVMContext context;
        std::unique_ptr< llvm::Module > module;
        ir_builder_t builder;
        std::unique_ptr< llvm::TargetMachine > target_machine;
        std::map< quxlang::type_symbol, llvm::Function* > functions;
        std::map< quxlang::type_symbol, callable_abi > function_abis;
        std::map< quxlang::type_symbol, llvm::GlobalVariable* > mutable_globals;
        std::map< quxlang::type_symbol, llvm::GlobalVariable* > constant_globals;
        std::map< quxlang::type_symbol, llvm::GlobalVariable* > initguard_globals;
        std::map< quxlang::type_symbol, llvm::StructType* > interface_structs;
        std::map< quxlang::type_symbol, std::map< quxlang::interface_slot_key, std::size_t > > interface_slot_indices;
        std::unique_ptr< llvm::DIBuilder > debug_builder;
        llvm::DICompileUnit* debug_compile_unit = nullptr;
        std::map< std::uint64_t, llvm::DIFile* > debug_files;
        std::map< quxlang::type_symbol, llvm::DISubprogram* > debug_subprograms;
        std::size_t helper_counter = 0;
        unsigned vmir2_metadata_kind = 0;
        std::optional< std::string > active_vmir2_metadata_text;
        std::uint64_t active_vmir2_metadata_counter = 0;

        /**
         * Ensures the LLVM target, MC, and asm subsystems needed for object emission are initialized once per process.
         */
        static void initialize_llvm_target_support(quxlang::machine_target_info const& machine)
        {
            switch (machine.cpu_type)
            {
            case quxlang::cpu::x86_32:
            case quxlang::cpu::x86_64:
            {
                static bool const initialized = []() -> bool
                {
                    ::LLVMInitializeX86TargetInfo();
                    ::LLVMInitializeX86Target();
                    ::LLVMInitializeX86TargetMC();
                    ::LLVMInitializeX86AsmParser();
                    ::LLVMInitializeX86AsmPrinter();
                    return true;
                }();
                (void)initialized;
                return;
            }
            case quxlang::cpu::arm_32:
            {
                static bool const initialized = []() -> bool
                {
                    ::LLVMInitializeARMTargetInfo();
                    ::LLVMInitializeARMTarget();
                    ::LLVMInitializeARMTargetMC();
                    ::LLVMInitializeARMAsmParser();
                    ::LLVMInitializeARMAsmPrinter();
                    return true;
                }();
                (void)initialized;
                return;
            }
            case quxlang::cpu::arm_64:
            {
                static bool const initialized = []() -> bool
                {
                    ::LLVMInitializeAArch64TargetInfo();
                    ::LLVMInitializeAArch64Target();
                    ::LLVMInitializeAArch64TargetMC();
                    ::LLVMInitializeAArch64AsmParser();
                    ::LLVMInitializeAArch64AsmPrinter();
                    return true;
                }();
                (void)initialized;
                return;
            }
            case quxlang::cpu::riscv_32:
            case quxlang::cpu::riscv_64:
            {
                static bool const initialized = []() -> bool
                {
                    ::LLVMInitializeRISCVTargetInfo();
                    ::LLVMInitializeRISCVTarget();
                    ::LLVMInitializeRISCVTargetMC();
                    ::LLVMInitializeRISCVAsmParser();
                    ::LLVMInitializeRISCVAsmPrinter();
                    return true;
                }();
                (void)initialized;
                return;
            }
            case quxlang::cpu::none:
                break;
            }

            throw quxlang::semantic_compilation_error("Unsupported LLVM target initialization CPU kind");
        }

        /**
         * Returns the LLVM machine-code optimization level for one Quxlang LLVM backend mode.
         */
        static auto llvm_codegen_opt_level(quxlang::llvm_backend::optimization_level optimization) -> llvm::CodeGenOptLevel
        {
            switch (optimization)
            {
            case quxlang::llvm_backend::optimization_level::debug:
                return llvm::CodeGenOptLevel::None;
            case quxlang::llvm_backend::optimization_level::release:
                return llvm::CodeGenOptLevel::Default;
            }

            throw quxlang::compiler_bug("Unsupported LLVM backend optimization level");
        }

        /**
         * Creates one LLVM target machine for the requested qxc machine target and optimization mode.
         */
        static auto create_target_machine(
            quxlang::machine_target_info const& machine,
            quxlang::llvm_backend::optimization_level optimization) -> std::unique_ptr< llvm::TargetMachine >
        {
            initialize_llvm_target_support(machine);

            std::string const triple_text = quxlang::lookup_llvm_triple(machine);
            llvm::Triple triple(triple_text);
            std::string target_error;
            llvm::Target const* target = llvm::TargetRegistry::lookupTarget(triple, target_error);
            if (target == nullptr)
            {
                throw quxlang::semantic_compilation_error("Failed to lookup LLVM target for " + triple_text + ": " + target_error);
            }

            llvm::TargetOptions options;
            options.ExceptionModel = llvm::ExceptionHandling::DwarfCFI;
            llvm::Reloc::Model reloc_model = llvm::Reloc::Model::Static;
            llvm::CodeModel::Model code_model = llvm::CodeModel::Medium;
            if (machine.cpu_type == quxlang::cpu::arm_64 || machine.cpu_type == quxlang::cpu::x86_64 || machine.cpu_type == quxlang::cpu::riscv_64)
            {
                code_model = llvm::CodeModel::Large;
            }

            llvm::CodeGenOptLevel const opt_level = llvm_codegen_opt_level(optimization);
            llvm::TargetMachine* raw_machine = target->createTargetMachine(
                triple,
                "generic",
                "",
                options,
                reloc_model,
                code_model,
                opt_level);
            if (raw_machine == nullptr)
            {
                throw quxlang::semantic_compilation_error("Failed to create LLVM target machine for " + triple_text);
            }
            return std::unique_ptr< llvm::TargetMachine >(raw_machine);
        }

        /**
         * Emits one LLVM module to a target object file byte buffer.
         */
        auto emit_module_object_file(
            llvm::Module const& source_module,
            quxlang::llvm_backend::optimization_level optimization) -> std::vector< std::byte >
        {
            std::unique_ptr< llvm::TargetMachine > object_target_machine = create_target_machine(input.machine_target.machine, optimization);
            std::unique_ptr< llvm::Module > object_module = llvm::CloneModule(source_module);
            object_module->setTargetTriple(module->getTargetTriple());
            object_module->setDataLayout(object_target_machine->createDataLayout());

            llvm::SmallVector< char, 0 > object_buffer;
            llvm::raw_svector_ostream object_stream(object_buffer);
            llvm::legacy::PassManager pass_manager;
            if (object_target_machine->addPassesToEmitFile(pass_manager, object_stream, nullptr, llvm::CodeGenFileType::ObjectFile))
            {
                throw quxlang::semantic_compilation_error("Failed to emit LLVM object file for " + quxlang::to_string(input.target_name));
            }
            pass_manager.run(*object_module);

            std::vector< std::byte > result;
            result.resize(object_buffer.size());
            for (std::size_t i = 0; i < object_buffer.size(); i++)
            {
                result[i] = static_cast< std::byte >(object_buffer[i]);
            }
            return result;
        }

        /**
         * Returns true when this aggregate LLVM packet should synthesize a Linux ELF process entrypoint.
         */
        auto should_emit_linux_start() const -> bool
        {
            return input.whole_module && input.whole_module_output_kind == quxlang::output_kind::executable && input.machine_target.machine.os_type == quxlang::os::linux &&
                   input.machine_target.machine.binary_type == quxlang::binary::elf && !input.executable_entry_symbol.has_value();
        }

        /**
         * Emits one Linux `_start` routine that calls the selected Qux main function and exits with its return code.
         */
        void emit_linux_start()
        {
            if (module->getFunction("_start") != nullptr)
            {
                throw quxlang::semantic_compilation_error("LLVM lowering attempted to redefine _start for " + quxlang::to_string(input.target_name));
            }

            llvm::Function* const main_function = declared_function(input.target_name);
            if (main_function->arg_size() != 0)
            {
                throw quxlang::semantic_compilation_error("Executable entry functanoid must not require arguments: " + quxlang::to_string(input.target_name));
            }

            llvm::Function* const start_function =
                llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), false), llvm::GlobalValue::ExternalLinkage, "_start", module.get());
            start_function->setDoesNotThrow();
            start_function->addFnAttr(llvm::Attribute::NoReturn);

            llvm::BasicBlock* const entry_block = llvm::BasicBlock::Create(context, "entry", start_function);
            builder.SetInsertPoint(entry_block);

            llvm::Value* exit_code_value = nullptr;
            if (main_function->getReturnType()->isVoidTy())
            {
                builder.CreateCall(main_function, {});
                exit_code_value = llvm::ConstantInt::get(pointer_integer_type(), 0);
            }
            else if (main_function->getReturnType()->isIntegerTy())
            {
                llvm::Value* const result = builder.CreateCall(main_function, {});
                if (result->getType() == pointer_integer_type())
                {
                    exit_code_value = result;
                }
                else if (result->getType()->getIntegerBitWidth() < pointer_integer_type()->getIntegerBitWidth())
                {
                    exit_code_value = builder.CreateZExt(result, pointer_integer_type());
                }
                else if (result->getType()->getIntegerBitWidth() > pointer_integer_type()->getIntegerBitWidth())
                {
                    exit_code_value = builder.CreateTrunc(result, pointer_integer_type());
                }
                else
                {
                    exit_code_value = result;
                }
            }
            else
            {
                throw quxlang::semantic_compilation_error("Executable entry functanoid must return VOID or an integer-like value: " + quxlang::to_string(input.target_name));
            }

            emit_linux_exit_syscall(exit_code_value);
            builder.CreateUnreachable();
        }

        /**
         * Emits the architecture-specific Linux process-exit syscall sequence for one exit code value.
         */
        void emit_linux_exit_syscall(llvm::Value* exit_code_value)
        {
            switch (input.machine_target.machine.cpu_type)
            {
            case quxlang::cpu::x86_64:
            {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(
                    exit_type,
                    "movq $$60, %rax\n\tsyscall\n\tud2",
                    "{rdi},~{rax},~{rcx},~{r11},~{memory}",
                    true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            case quxlang::cpu::x86_32:
            {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(
                    exit_type,
                    "movl $$1, %eax\n\tint $$0x80\n\tud2",
                    "{ebx},~{eax},~{memory}",
                    true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            case quxlang::cpu::arm_64:
            {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(
                    exit_type,
                    "mov x8, #93\n\tsvc #0\n\tbrk #1",
                    "{x0},~{x8},~{memory}",
                    true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            default:
                break;
            }

            throw quxlang::semantic_compilation_error("Linux ELF _start lowering is not implemented for this CPU kind");
        }

        /**
         * Attaches !qux.vmir2 metadata to one newly inserted LLVM instruction while a VMIR lowering context is active.
         */
        void annotate_inserted_instruction(llvm::Instruction* instruction)
        {
            if (!active_vmir2_metadata_text.has_value())
            {
                return;
            }

            llvm::Metadata* metadata_values[] = {
                llvm::MDString::get(context, *active_vmir2_metadata_text),
                llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(i64_type(), active_vmir2_metadata_counter++)),
            };
            instruction->setMetadata(vmir2_metadata_kind, llvm::MDNode::get(context, metadata_values));
        }

        /**
         * Begins annotating subsequently inserted LLVM instructions as part of one VMIR instruction or terminator lowering.
         */
        void begin_vmir2_metadata_context(std::string text)
        {
            active_vmir2_metadata_text = std::move(text);
            active_vmir2_metadata_counter = 0;
        }

        /**
         * Stops annotating subsequently inserted LLVM instructions with !qux.vmir2 metadata.
         */
        void end_vmir2_metadata_context()
        {
            active_vmir2_metadata_text.reset();
            active_vmir2_metadata_counter = 0;
        }

        /**
         * Formats one VMIR instruction exactly once for use in the custom !qux.vmir2 metadata payload.
         */
        auto vmir2_metadata_text(quxlang::vmir2::functanoid_routine3 const& routine, quxlang::vmir2::vm_instruction const& instruction) const -> std::string
        {
            quxlang::vmir2::assembler formatter(routine);
            formatter.print_comments = false;
            return formatter.to_string(instruction);
        }

        /**
         * Formats one VMIR terminator exactly once for use in the custom !qux.vmir2 metadata payload.
         */
        auto vmir2_metadata_text(quxlang::vmir2::functanoid_routine3 const& routine, quxlang::vmir2::vm_terminator const& terminator) const -> std::string
        {
            quxlang::vmir2::assembler formatter(routine);
            formatter.print_comments = false;
            return formatter.to_string(terminator);
        }

        auto local_slot_index(quxlang::vmir2::local_index value) const -> std::size_t
        {
            return static_cast< std::size_t >(std::uint64_t(value));
        }

        auto block_slot_index(quxlang::vmir2::block_index value) const -> std::size_t
        {
            return static_cast< std::size_t >(std::uint64_t(value));
        }

        auto opaque_pointer_type() -> llvm::PointerType*
        {
            return llvm::PointerType::get(context, 0);
        }

        auto i8_type() -> llvm::IntegerType*
        {
            return llvm::Type::getInt8Ty(context);
        }

        auto i64_type() -> llvm::IntegerType*
        {
            return llvm::Type::getInt64Ty(context);
        }

        auto pointer_integer_type() -> llvm::IntegerType*
        {
            return llvm::IntegerType::get(context, input.machine_target.machine.pointer_size_bytes() * 8);
        }

        /**
         * Returns one already-declared LLVM function by its Qux symbol.
         */
        auto declared_function(quxlang::type_symbol const& symbol) const -> llvm::Function*
        {
            std::map< quxlang::type_symbol, llvm::Function* >::const_iterator function_iter = functions.find(symbol);
            if (function_iter == functions.end())
            {
                throw quxlang::semantic_compilation_error("Missing declared LLVM function for " + quxlang::to_string(symbol));
            }
            return function_iter->second;
        }

        auto bool_storage_type() -> llvm::IntegerType*
        {
            return i8_type();
        }

        /**
         * Returns the backend-owned placement for the internal array-initializer runtime record.
         */
        auto array_initializer_storage_placement() const -> quxlang::type_placement_info
        {
            std::uint64_t const pointer_size = input.machine_target.machine.pointer_size_bytes();
            std::uint64_t const pointer_align = input.machine_target.machine.pointer_align();
            std::uint64_t const integer64_size = 8;
            std::uint64_t const integer64_align = input.machine_target.machine.integer_alignment_for_bits(64);

            std::uint64_t const record_align = std::max(pointer_align, integer64_align);
            std::uint64_t record_size = pointer_size;
            if (record_size % integer64_align != 0)
            {
                record_size += integer64_align - (record_size % integer64_align);
            }
            record_size += integer64_size;
            if (record_size % integer64_align != 0)
            {
                record_size += integer64_align - (record_size % integer64_align);
            }
            record_size += integer64_size;
            if (record_size % record_align != 0)
            {
                record_size += record_align - (record_size % record_align);
            }

            return quxlang::type_placement_info{.size = record_size, .alignment = record_align};
        }

        /**
         * Produces the integer key used for standard Qux float comparisons.
         *
         * This implements the existing strong ordering model:
         * negative values sort before positive values, `-0` sorts before `+0`,
         * and canonical NaNs sort after all non-NaN values. Non-canonical NaN
         * payloads are assumed to be undefined behavior before LLVM lowering.
         */
        auto float_total_order_key(llvm::Value* value, quxlang::float_type const& float_info, ir_builder_t& ir_builder) -> llvm::Value*
        {
            llvm::IntegerType* integer_type = llvm::IntegerType::get(context, float_info.bits);
            llvm::Value* raw_bits = ir_builder.CreateBitCast(value, integer_type);
            llvm::ConstantInt* sign_mask = llvm::cast< llvm::ConstantInt >(llvm::ConstantInt::get(integer_type, llvm::APInt::getOneBitSet(float_info.bits, float_info.bits - 1)));
            llvm::ConstantInt* zero = llvm::cast< llvm::ConstantInt >(llvm::ConstantInt::get(integer_type, llvm::APInt(float_info.bits, 0)));
            llvm::Value* is_negative = ir_builder.CreateICmpNE(ir_builder.CreateAnd(raw_bits, sign_mask), zero);
            llvm::Value* negative_key = ir_builder.CreateNot(raw_bits);
            llvm::Value* nonnegative_key = ir_builder.CreateXor(raw_bits, sign_mask);
            return ir_builder.CreateSelect(is_negative, negative_key, nonnegative_key);
        }

        auto byte_array_type(std::uint64_t size) -> llvm::ArrayType*
        {
            return llvm::ArrayType::get(i8_type(), std::max< std::uint64_t >(size, 1));
        }

        auto output_slot_target(quxlang::type_symbol const& type) const -> std::optional< quxlang::type_symbol >
        {
            if (type.type_is< quxlang::nvalue_slot >())
            {
                return type.get_as< quxlang::nvalue_slot >().target;
            }
            if (type.type_is< quxlang::dvalue_slot >())
            {
                return type.get_as< quxlang::dvalue_slot >().target;
            }
            return std::nullopt;
        }

        auto is_output_slot_type(quxlang::type_symbol const& type) const -> bool
        {
            return output_slot_target(type).has_value();
        }

        auto is_pointer_valued_type(quxlang::type_symbol const& type) const -> bool
        {
            return quxlang::is_ref(type) || quxlang::is_ptr(type) || type.type_is< quxlang::procedure_type >() || type.type_is< quxlang::initguard_lock_type >() || interface_runtime_type(type);
        }

        auto interface_runtime_type(quxlang::type_symbol const& type) const -> bool
        {
            return input.interface_slots.contains(type);
        }

        auto enum_runtime_type(quxlang::type_symbol const& type) const -> bool
        {
            return input.enum_infos.contains(type);
        }

        auto flagset_runtime_type(quxlang::type_symbol const& type) const -> bool
        {
            return input.flagset_infos.contains(type);
        }

        auto nominal_integer_runtime_type(quxlang::type_symbol const& type) const -> bool
        {
            return enum_runtime_type(type) || flagset_runtime_type(type);
        }

        auto nominal_integer_bit_width(quxlang::type_symbol const& type) const -> std::optional< unsigned >
        {
            std::map< quxlang::type_symbol, quxlang::enum_info >::const_iterator enum_iter = input.enum_infos.find(type);
            if (enum_iter != input.enum_infos.end())
            {
                return static_cast< unsigned >(enum_iter->second.bits);
            }

            std::map< quxlang::type_symbol, quxlang::flagset_info >::const_iterator flagset_iter = input.flagset_infos.find(type);
            if (flagset_iter != input.flagset_infos.end())
            {
                return static_cast< unsigned >(flagset_iter->second.bits);
            }

            return std::nullopt;
        }

        /**
         * Returns true when the runtime value crosses the LLVM boundary directly instead of by storage pointer.
         */
        auto abi_passes_by_value(quxlang::type_symbol const& type) const -> bool
        {
            if (type.type_is< quxlang::attached_type_reference >())
            {
                quxlang::attached_type_reference const& attached = type.get_as< quxlang::attached_type_reference >();
                if (attached.carrying_type.type_is< quxlang::void_type >())
                {
                    return false;
                }
                return abi_passes_by_value(attached.carrying_type);
            }

            if (type.type_is< quxlang::bool_type >() || type.type_is< quxlang::byte_type >() || type.type_is< quxlang::int_type >() || type.type_is< quxlang::size_type >() ||
                type.type_is< quxlang::float_type >() || type.type_is< quxlang::procedure_type >() || type.type_is< quxlang::initguard_lock_type >())
            {
                return true;
            }

            if (nominal_integer_runtime_type(type))
            {
                return true;
            }

            if (quxlang::is_ref(type) || quxlang::is_ptr(type) || interface_runtime_type(type))
            {
                return true;
            }

            return false;
        }

        /**
         * Returns the value type for one VMIR NEW slot when it is eligible to become the LLVM return type.
         */
        auto llvm_returnable_output_slot_target(quxlang::type_symbol const& type) const -> std::optional< quxlang::type_symbol >
        {
            if (!type.type_is< quxlang::nvalue_slot >())
            {
                return std::nullopt;
            }

            quxlang::type_symbol const& target = type.get_as< quxlang::nvalue_slot >().target;
            if (abi_passes_by_value(target))
            {
                return target;
            }
            return std::nullopt;
        }

        auto value_storage_type(quxlang::type_symbol const& type) -> llvm::Type*
        {
            if (std::optional< quxlang::type_symbol > const atomic_value_type = quxlang::atomic_type_argument(type); atomic_value_type.has_value())
            {
                if (atomic_value_type->type_is< quxlang::int_type >())
                {
                    return llvm::IntegerType::get(context, static_cast< unsigned >(slot_size(type) * 8));
                }
                return value_storage_type(*atomic_value_type);
            }

            if (type.type_is< quxlang::attached_type_reference >())
            {
                quxlang::attached_type_reference const& attached = type.get_as< quxlang::attached_type_reference >();
                if (attached.carrying_type.type_is< quxlang::void_type >())
                {
                    return byte_array_type(1);
                }
                return value_storage_type(attached.carrying_type);
            }

            if (type.type_is< quxlang::array_initializer_type >())
            {
                llvm::Type* fields[] = {opaque_pointer_type(), i64_type(), i64_type()};
                return llvm::StructType::get(context, llvm::ArrayRef< llvm::Type* >(fields));
            }
            if (type.type_is< quxlang::readonly_constant >())
            {
                llvm::Type* fields[] = {opaque_pointer_type(), opaque_pointer_type()};
                return llvm::StructType::get(context, llvm::ArrayRef< llvm::Type* >(fields));
            }
            if (type.type_is< quxlang::void_type >() || type.type_is< quxlang::constexpr_proxy >())
            {
                return byte_array_type(1);
            }
            if (type.type_is< quxlang::bool_type >())
            {
                return bool_storage_type();
            }
            if (type.type_is< quxlang::byte_type >())
            {
                return i8_type();
            }
            if (type.type_is< quxlang::int_type >())
            {
                quxlang::int_type const& int_info = type.get_as< quxlang::int_type >();
                return llvm::IntegerType::get(context, int_info.bits);
            }
            if (type.type_is< quxlang::size_type >())
            {
                return pointer_integer_type();
            }
            if (type.type_is< quxlang::float_type >())
            {
                quxlang::float_type const& float_info = type.get_as< quxlang::float_type >();
                if (float_info.bits == 16)
                {
                    return llvm::Type::getHalfTy(context);
                }
                if (float_info.bits == 32)
                {
                    return llvm::Type::getFloatTy(context);
                }
                if (float_info.bits == 64)
                {
                    return llvm::Type::getDoubleTy(context);
                }
                if (float_info.bits == 80)
                {
                    return llvm::Type::getX86_FP80Ty(context);
                }
                if (float_info.bits == 128)
                {
                    return llvm::Type::getFP128Ty(context);
                }
                throw quxlang::semantic_compilation_error("Unsupported float type for LLVM lowering: " + quxlang::to_string(type));
            }
            if (type.type_is< quxlang::initguard_type >())
            {
                return pointer_integer_type();
            }
            if (std::optional< unsigned > const nominal_bits = nominal_integer_bit_width(type); nominal_bits.has_value())
            {
                return llvm::IntegerType::get(context, *nominal_bits);
            }
            if (is_pointer_valued_type(type))
            {
                return opaque_pointer_type();
            }

            std::map< quxlang::type_symbol, quxlang::type_placement_info >::const_iterator placement_iter = input.type_placements.find(type);
            if (placement_iter == input.type_placements.end())
            {
                throw quxlang::semantic_compilation_error("Missing type placement for LLVM lowering: " + quxlang::to_string(type));
            }
            return byte_array_type(placement_iter->second.size);
        }

        auto abi_type(quxlang::type_symbol const& type) -> llvm::Type*
        {
            if (is_output_slot_type(type))
            {
                return opaque_pointer_type();
            }
            if (!abi_passes_by_value(type))
            {
                return opaque_pointer_type();
            }
            return value_storage_type(type);
        }

        /**
         * Returns true when one ABI parameter name is treated as a language keyword for ABI ordering.
         */
        auto is_keyword_parameter_name(std::string const& name) const -> bool
        {
            bool saw_alpha = false;
            for (char const ch : name)
            {
                unsigned char const uch = static_cast< unsigned char >(ch);
                if (std::isalpha(uch))
                {
                    saw_alpha = true;
                    if (!std::isupper(uch))
                    {
                        return false;
                    }
                    continue;
                }
                if (std::isdigit(uch) || ch == '_')
                {
                    continue;
                }
                return false;
            }
            return saw_alpha;
        }

        /**
         * Returns the ABI ordering category for one named parameter.
         */
        auto named_parameter_order_category(std::string const& name) const -> int
        {
            if (name == "THIS")
            {
                return 0;
            }
            if (name == "OTHER")
            {
                return 1;
            }
            if (is_keyword_parameter_name(name))
            {
                return 2;
            }
            return 3;
        }

        /**
         * Returns named parameters in ABI source order.
         */
        auto ordered_named_parameter_names(std::vector< std::string > names) const -> std::vector< std::string >
        {
            std::sort(names.begin(), names.end(), [this](std::string const& lhs, std::string const& rhs)
            {
                int const lhs_category = named_parameter_order_category(lhs);
                int const rhs_category = named_parameter_order_category(rhs);
                if (lhs_category != rhs_category)
                {
                    return lhs_category < rhs_category;
                }
                return lhs < rhs;
            });
            return names;
        }

        auto ordered_routine_parameters(quxlang::vmir2::functanoid_routine3 const& routine) const -> std::vector< routine_abi_parameter >
        {
            std::vector< routine_abi_parameter > result;
            result.reserve(routine.parameters.positional.size() + routine.parameters.named.size());

            std::vector< std::string > named_names;
            named_names.reserve(routine.parameters.named.size());
            for (std::pair< std::string const, quxlang::vmir2::routine_parameter > const& param : routine.parameters.named)
            {
                named_names.push_back(param.first);
            }

            for (std::string const& name : ordered_named_parameter_names(std::move(named_names)))
            {
                quxlang::vmir2::routine_parameter const& param = routine.parameters.named.at(name);
                result.push_back(routine_abi_parameter{
                    .name = name,
                    .positional_index = std::nullopt,
                    .parameter_type = param.type,
                    .local = param.local_index,
                });
            }

            for (std::size_t positional_index = 0; positional_index < routine.parameters.positional.size(); ++positional_index)
            {
                quxlang::vmir2::routine_parameter const& param = routine.parameters.positional[positional_index];
                result.push_back(routine_abi_parameter{
                    .name = std::nullopt,
                    .positional_index = positional_index,
                    .parameter_type = param.type,
                    .local = param.local_index,
                });
            }
            return result;
        }

        /**
         * Returns the readable LLVM argument name for one routine ABI parameter.
         */
        auto routine_argument_name(routine_abi_parameter const& param) const -> std::string
        {
            std::string source_name;
            if (param.name.has_value())
            {
                source_name = "arg_" + *param.name;
            }
            else
            {
                if (!param.positional_index.has_value())
                {
                    throw quxlang::semantic_compilation_error("Positional LLVM argument is missing its source index");
                }
                source_name = "arg_" + std::to_string(*param.positional_index);
            }

            if (is_output_slot_type(param.parameter_type) || !abi_passes_by_value(param.parameter_type))
            {
                return "slot" + std::to_string(local_slot_index(param.local)) + "_" + source_name;
            }
            return source_name;
        }

        auto select_llvm_return_source_index(std::vector< abi_parameter > const& ordered) const -> std::optional< std::size_t >
        {
            for (std::size_t i = 0; i < ordered.size(); ++i)
            {
                if (ordered[i].name == std::optional< std::string >{"RETURN"} && llvm_returnable_output_slot_target(ordered[i].type).has_value())
                {
                    return i;
                }
            }

            for (std::size_t i = 0; i < ordered.size(); ++i)
            {
                if (llvm_returnable_output_slot_target(ordered[i].type).has_value())
                {
                    return i;
                }
            }

            return std::nullopt;
        }

        auto build_callable_abi(std::vector< abi_parameter > ordered) -> callable_abi
        {
            callable_abi abi;
            abi.source_ordered = std::move(ordered);
            abi.return_source_index = select_llvm_return_source_index(abi.source_ordered);

            std::vector< llvm::Type* > param_types;
            param_types.reserve(abi.source_ordered.size());
            for (std::size_t i = 0; i < abi.source_ordered.size(); ++i)
            {
                abi_parameter const& param = abi.source_ordered[i];
                if (abi.return_source_index.has_value() && *abi.return_source_index == i)
                {
                    if (param.name.has_value())
                    {
                        abi.source_named_indices[*param.name] = i;
                    }
                    continue;
                }

                abi.llvm_param_source_indices.push_back(i);
                param_types.push_back(abi_type(param.type));
                if (param.name.has_value())
                {
                    abi.source_named_indices[*param.name] = i;
                }
            }

            llvm::Type* return_type = llvm::Type::getVoidTy(context);
            if (abi.return_source_index.has_value())
            {
                return_type = value_storage_type(*llvm_returnable_output_slot_target(abi.source_ordered.at(*abi.return_source_index).type));
            }

            abi.llvm_type = llvm::FunctionType::get(return_type, param_types, false);
            return abi;
        }

        /**
         * Maps one source-level calling convention tag to the corresponding LLVM call convention.
         */
        auto llvm_calling_convention(std::string const& calling_convention) const -> llvm::CallingConv::ID
        {
            std::string const normalized = upper_ascii(calling_convention);
            if (normalized == "DEFAULT" || normalized == "CCALL")
            {
                return llvm::CallingConv::C;
            }
            if (normalized == "STDCALL")
            {
                return llvm::CallingConv::X86_StdCall;
            }
            throw quxlang::semantic_compilation_error("Unsupported LLVM calling convention: " + calling_convention);
        }

        /**
         * Applies one ABI's calling convention to an LLVM function declaration or definition.
         */
        void apply_calling_convention(llvm::Function* function, callable_abi const& abi) const
        {
            function->setCallingConv(llvm_calling_convention(abi.calling_convention));
        }

        /**
         * Applies one ABI's calling convention to an LLVM call instruction.
         */
        void apply_calling_convention(llvm::CallInst* call, callable_abi const& abi) const
        {
            call->setCallingConv(llvm_calling_convention(abi.calling_convention));
        }

        auto callable_abi_from_routine(quxlang::vmir2::functanoid_routine3 const& routine) -> callable_abi
        {
            std::vector< routine_abi_parameter > routine_params = ordered_routine_parameters(routine);
            std::vector< abi_parameter > ordered;
            ordered.reserve(routine_params.size());
            for (routine_abi_parameter const& param : routine_params)
            {
                ordered.push_back(abi_parameter{
                    .name = param.name,
                    .positional_index = param.positional_index,
                    .type = param.parameter_type,
                });
            }
            return build_callable_abi(std::move(ordered));
        }

        /**
         * Returns the concrete runtime type for one instantiated formal parameter, omitting purely compile-time payloads.
         */
        auto runtime_parameter_type(quxlang::type_symbol const& parameter_type) const -> std::optional< quxlang::type_symbol >
        {
            if (!parameter_type.type_is< quxlang::attached_type_reference >())
            {
                return parameter_type;
            }

            quxlang::attached_type_reference const& attached = parameter_type.get_as< quxlang::attached_type_reference >();
            if (attached.carrying_type.type_is< quxlang::void_type >())
            {
                return std::nullopt;
            }
            return attached.carrying_type;
        }

        auto callable_abi_from_signature(quxlang::sigtype const& signature) -> callable_abi
        {
            std::vector< abi_parameter > ordered;
            ordered.reserve(signature.params.positional.size() + signature.params.named.size() + (signature.return_type.has_value() ? 1 : 0));
            std::map< std::string, quxlang::type_symbol > named = signature.params.named;
            if (signature.return_type.has_value() && !signature.return_type->type_is< quxlang::void_type >())
            {
                named["RETURN"] = quxlang::nvalue_slot{.target = *signature.return_type};
            }
            std::vector< std::string > named_names;
            named_names.reserve(named.size());
            for (std::pair< std::string const, quxlang::type_symbol > const& param : named)
            {
                named_names.push_back(param.first);
            }
            for (std::string const& name : ordered_named_parameter_names(std::move(named_names)))
            {
                ordered.push_back(abi_parameter{
                    .name = name,
                    .positional_index = std::nullopt,
                    .type = named.at(name),
                });
            }
            for (std::size_t positional_index = 0; positional_index < signature.params.positional.size(); ++positional_index)
            {
                ordered.push_back(abi_parameter{
                    .name = std::nullopt,
                    .positional_index = positional_index,
                    .type = signature.params.positional[positional_index],
                });
            }
            return build_callable_abi(std::move(ordered));
        }

        /**
         * Reconstructs a callable ABI from a concrete instantiated symbol plus an optional runtime return slot.
         */
        auto callable_abi_from_instanciation_reference(quxlang::instanciation_reference const& inst, std::optional< quxlang::type_symbol > return_slot_type) -> callable_abi
        {
            std::vector< abi_parameter > ordered;
            ordered.reserve(inst.params.positional.size() + inst.params.named.size() + (return_slot_type.has_value() ? 1 : 0));

            std::map< std::string, quxlang::type_symbol > named;
            for (std::pair< std::string const, quxlang::parameter_instantiation > const& param : inst.params.named)
            {
                std::optional< quxlang::type_symbol > runtime_type = runtime_parameter_type(quxlang::parameter_instantiation_type(param.second));
                if (!runtime_type.has_value())
                {
                    continue;
                }
                named.emplace(param.first, *runtime_type);
            }
            if (return_slot_type.has_value() && !return_slot_type->type_is< quxlang::void_type >())
            {
                named["RETURN"] = quxlang::nvalue_slot{.target = *return_slot_type};
            }

            std::vector< std::string > named_names;
            named_names.reserve(named.size());
            for (std::pair< std::string const, quxlang::type_symbol > const& param : named)
            {
                named_names.push_back(param.first);
            }
            for (std::string const& name : ordered_named_parameter_names(std::move(named_names)))
            {
                ordered.push_back(abi_parameter{
                    .name = name,
                    .positional_index = std::nullopt,
                    .type = named.at(name),
                });
            }

            std::size_t runtime_positional_index = 0;
            for (quxlang::parameter_instantiation const& positional : inst.params.positional)
            {
                std::optional< quxlang::type_symbol > runtime_type = runtime_parameter_type(quxlang::parameter_instantiation_type(positional));
                if (!runtime_type.has_value())
                {
                    continue;
                }
                ordered.push_back(abi_parameter{
                    .name = std::nullopt,
                    .positional_index = runtime_positional_index,
                    .type = *runtime_type,
                });
                runtime_positional_index++;
            }

            return build_callable_abi(std::move(ordered));
        }

        auto callable_abi_from_invoke(quxlang::vmir2::invoke const& call, function_codegen_state const& state) -> callable_abi
        {
            std::vector< abi_parameter > ordered;
            ordered.reserve(call.args.positional.size() + call.args.named.size());
            std::map< std::string, quxlang::type_symbol > named;
            for (std::pair< std::string const, quxlang::vmir2::local_index > const& arg : call.args.named)
            {
                quxlang::type_symbol arg_type = state.routine->local_types.at(local_slot_index(arg.second)).type;
                if (arg.first == "RETURN")
                {
                    arg_type = quxlang::nvalue_slot{.target = arg_type};
                }
                named[arg.first] = arg_type;
            }
            std::vector< std::string > named_names;
            named_names.reserve(named.size());
            for (std::pair< std::string const, quxlang::type_symbol > const& arg : named)
            {
                named_names.push_back(arg.first);
            }
            for (std::string const& name : ordered_named_parameter_names(std::move(named_names)))
            {
                ordered.push_back(abi_parameter{
                    .name = name,
                    .positional_index = std::nullopt,
                    .type = named.at(name),
                });
            }
            for (std::size_t positional_index = 0; positional_index < call.args.positional.size(); ++positional_index)
            {
                quxlang::vmir2::local_index const arg = call.args.positional[positional_index];
                ordered.push_back(abi_parameter{
                    .name = std::nullopt,
                    .positional_index = positional_index,
                    .type = state.routine->local_types.at(local_slot_index(arg)).type,
                });
            }
            return build_callable_abi(std::move(ordered));
        }

        /**
         * Returns the emitted linker-visible symbol name for one procedure symbol.
         */
        auto symbol_link_name(quxlang::type_symbol const& symbol) const -> std::string
        {
            std::map< quxlang::type_symbol, std::string >::const_iterator const found = input.procedure_linksymbols.find(symbol);
            if (found != input.procedure_linksymbols.end())
            {
                return found->second;
            }
            return quxlang::to_string(symbol);
        }

        /**
         * Returns the concrete symbol initialized for one runtime procedure reference.
         */
        auto runtime_procedure_symbol(quxlang::llvm_backend::runtime_procedure_reference const& reference) const -> quxlang::type_symbol const&
        {
            std::map< quxlang::llvm_backend::runtime_procedure_reference, quxlang::type_symbol >::const_iterator const found = input.runtime_procedures.find(reference);
            if (found == input.runtime_procedures.end())
            {
                throw quxlang::semantic_compilation_error("Missing initialized runtime procedure for LLVM lowering");
            }
            return found->second;
        }

        /**
         * Converts one stored asm routine into textual assembler for the current target.
         */
        auto assembly_text(quxlang::asm_procedure const& procedure) const -> std::string
        {
            if (procedure.architecture == "ARM")
            {
                return quxlang::convert_to_arm_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name);
            }
            if (procedure.architecture == "X64")
            {
                return quxlang::convert_to_x64_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name);
            }
            throw quxlang::semantic_compilation_error("Unsupported asm procedure architecture for LLVM lowering: " + procedure.architecture);
        }

        /**
         * Uppercases ASCII letters in a copy of the input string.
         */
        auto upper_ascii(std::string text) const -> std::string
        {
            for (char& ch : text)
            {
                ch = static_cast< char >(std::toupper(static_cast< unsigned char >(ch)));
            }
            return text;
        }

        auto callable_abi_from_asm_callable(quxlang::asm_callable const& callable) -> callable_abi
        {
            std::vector< abi_parameter > ordered;
            ordered.reserve(callable.args.size());
            for (std::size_t i = 0; i < callable.args.size(); ++i)
            {
                ordered.push_back(abi_parameter{
                    .name = std::nullopt,
                    .positional_index = i,
                    .type = callable.args.at(i).type,
                });
            }
            if (callable.return_type.has_value())
            {
                ordered.push_back(abi_parameter{
                    .name = "RETURN",
                    .positional_index = std::nullopt,
                    .type = quxlang::nvalue_slot{.target = *callable.return_type},
                });
            }

            callable_abi abi = build_callable_abi(std::move(ordered));
            abi.calling_convention = callable.calling_conv;
            return abi;
        }

        void declare_defined_function(quxlang::type_symbol const& symbol, quxlang::vmir2::functanoid_routine3 const& routine, llvm::GlobalValue::LinkageTypes linkage)
        {
            callable_abi abi = callable_abi_from_routine(routine);
            llvm::Function* function = llvm::Function::Create(abi.llvm_type, linkage, symbol_link_name(symbol), module.get());
            apply_calling_convention(function, abi);
            functions[symbol] = function;
            function_abis[symbol] = abi;
        }

        void declare_asm_callable_function(quxlang::type_symbol const& symbol, quxlang::asm_callable const& callable)
        {
            callable_abi abi = callable_abi_from_asm_callable(callable);
            std::string const link_name = symbol_link_name(symbol);
            llvm::Function* function = module->getFunction(link_name);
            if (function == nullptr)
            {
                function = llvm::Function::Create(abi.llvm_type, llvm::GlobalValue::ExternalLinkage, link_name, module.get());
            }
            apply_calling_convention(function, abi);
            functions[symbol] = function;
            function_abis[symbol] = abi;
        }

        auto get_or_create_external_function(quxlang::type_symbol const& symbol, callable_abi const& abi) -> llvm::Function*
        {
            std::map< quxlang::type_symbol, llvm::Function* >::const_iterator existing = functions.find(symbol);
            if (existing != functions.end())
            {
                return existing->second;
            }

            std::string const link_name = symbol_link_name(symbol);
            llvm::Function* function = module->getFunction(link_name);
            if (function == nullptr)
            {
                function = llvm::Function::Create(abi.llvm_type, llvm::GlobalValue::ExternalLinkage, link_name, module.get());
            }
            apply_calling_convention(function, abi);
            functions[symbol] = function;
            function_abis[symbol] = abi;
            return function;
        }

        auto get_or_create_malloc() -> llvm::Function*
        {
            llvm::FunctionType* function_type = llvm::FunctionType::get(opaque_pointer_type(), {i64_type()}, false);
            return llvm::cast< llvm::Function >(module->getOrInsertFunction("malloc", function_type).getCallee());
        }

        auto get_or_create_free() -> llvm::Function*
        {
            llvm::FunctionType* function_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {opaque_pointer_type()}, false);
            return llvm::cast< llvm::Function >(module->getOrInsertFunction("free", function_type).getCallee());
        }

        /**
         * Builds the concrete LLVM callable ABI for one initguard runtime procedure.
         */
        auto initguard_runtime_abi(quxlang::llvm_backend::runtime_procedure procedure) -> callable_abi
        {
            std::vector< abi_parameter > ordered;
            ordered.push_back(abi_parameter{
                .name = "guard",
                .positional_index = std::nullopt,
                .type = quxlang::ptrref_type{
                    .target = quxlang::initguard_type{},
                    .ptr_class = quxlang::pointer_class::ref,
                    .qual = quxlang::qualifier::mut,
                },
            });

            std::optional< quxlang::type_symbol > const return_type = quxlang::llvm_backend::runtime_procedure_return_type(procedure);
            if (return_type.has_value())
            {
                ordered.push_back(abi_parameter{
                    .name = "RETURN",
                    .positional_index = std::nullopt,
                    .type = quxlang::nvalue_slot{.target = *return_type},
                });
            }

            return build_callable_abi(std::move(ordered));
        }

        /**
         * Resolves one abstract initguard runtime procedure to its concrete external LLVM function.
         */
        auto get_or_create_initguard_runtime_function(quxlang::llvm_backend::runtime_procedure procedure, callable_abi const& abi) -> llvm::Function*
        {
            quxlang::llvm_backend::runtime_procedure_reference const reference{.procedure = procedure};
            quxlang::type_symbol const& symbol = runtime_procedure_symbol(reference);
            return get_or_create_external_function(symbol, abi);
        }

        /**
         * Converts a little-endian raw byte buffer into an APInt of the requested width.
         */
        auto little_endian_apint(std::vector< std::byte > const& bytes, unsigned bit_width) const -> llvm::APInt
        {
            std::size_t const required_bytes = std::max< std::size_t >((bit_width + 7) / 8, 1);
            if (bytes.size() > required_bytes)
            {
                throw quxlang::semantic_compilation_error("antestatal primitive payload exceeds LLVM storage width");
            }

            std::vector< std::uint64_t > words((bit_width + 63) / 64, 0);
            for (std::size_t i = 0; i < bytes.size(); ++i)
            {
                std::size_t const word_index = i / 8;
                std::size_t const byte_offset = i % 8;
                words[word_index] |= static_cast< std::uint64_t >(std::to_integer< std::uint8_t >(bytes[i])) << (byte_offset * 8);
            }

            return llvm::APInt(bit_width, llvm::ArrayRef< std::uint64_t >(words));
        }

        /**
         * Normalizes a raw byte payload to the exact storage width required by LLVM lowering.
         */
        auto normalized_raw_bytes(std::vector< std::byte > const& bytes, std::size_t storage_size) const -> std::vector< std::byte >
        {
            if (bytes.size() > storage_size)
            {
                throw quxlang::semantic_compilation_error("antestatal value exceeds declared storage size");
            }

            std::vector< std::byte > result = bytes;
            result.resize(storage_size, std::byte{0});
            return result;
        }

        /**
         * Builds a byte-array LLVM constant from raw storage bytes.
         */
        auto constant_byte_array(std::vector< std::byte > const& bytes) -> llvm::Constant*
        {
            std::vector< std::uint8_t > raw_bytes;
            raw_bytes.reserve(bytes.size());
            for (std::byte byte : bytes)
            {
                raw_bytes.push_back(std::to_integer< std::uint8_t >(byte));
            }
            return llvm::ConstantDataArray::get(context, llvm::ArrayRef< std::uint8_t >(raw_bytes));
        }

        /**
         * Materializes a private immutable byte buffer for readonly constant payloads referenced from VMIR slots.
         */
        auto create_private_constant_bytes_global(std::vector< std::byte > const& bytes, std::string const& name_stem) -> llvm::GlobalVariable*
        {
            std::vector< std::byte > storage_bytes = bytes;
            if (storage_bytes.empty())
            {
                storage_bytes.push_back(std::byte{0});
            }

            llvm::Constant* initializer = constant_byte_array(storage_bytes);
            llvm::GlobalVariable* global = new llvm::GlobalVariable(
                *module,
                initializer->getType(),
                true,
                llvm::GlobalValue::PrivateLinkage,
                initializer,
                name_stem + "$constbytes$" + std::to_string(helper_counter++));
            global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            global->setAlignment(llvm::Align(1));
            return global;
        }

        /**
         * Flattens a readonly antestatal value into raw storage bytes for byte-addressed aggregates.
         */
        auto materialize_antestatal_bytes(quxlang::type_symbol const& type, quxlang::antestatal_value const& value) -> std::vector< std::byte >
        {
            std::size_t const storage_size = slot_size(type);

            if (value.type_is< quxlang::antestatal_primitive >())
            {
                return normalized_raw_bytes(value.get_as< quxlang::antestatal_primitive >().value, storage_size);
            }

            if (type.type_is< quxlang::array_type >() && value.type_is< quxlang::antestatal_array >())
            {
                quxlang::array_type const& array_type = type.get_as< quxlang::array_type >();
                quxlang::antestatal_array const& array_value = value.get_as< quxlang::antestatal_array >();
                std::vector< std::byte > result(storage_size, std::byte{0});
                std::uint64_t const element_size = slot_size(array_type.element_type);
                for (std::size_t i = 0; i < array_value.elements.size(); ++i)
                {
                    std::uint64_t const offset = static_cast< std::uint64_t >(i) * element_size;
                    if (offset + element_size > result.size())
                    {
                        throw quxlang::semantic_compilation_error("antestatal array initializer exceeds declared storage size");
                    }

                    std::vector< std::byte > const element_bytes = materialize_antestatal_bytes(array_type.element_type, array_value.elements[i]);
                    std::copy(element_bytes.begin(), element_bytes.end(), result.begin() + static_cast< std::ptrdiff_t >(offset));
                }
                return result;
            }

            if (value.type_is< quxlang::antestatal_struct >())
            {
                std::map< quxlang::type_symbol, quxlang::class_layout >::const_iterator layout_iter = input.class_layouts.find(type);
                if (layout_iter == input.class_layouts.end())
                {
                    throw quxlang::semantic_compilation_error("Missing class layout for readonly antestatal constant: " + quxlang::to_string(type));
                }

                quxlang::antestatal_struct const& struct_value = value.get_as< quxlang::antestatal_struct >();
                std::vector< std::byte > result(storage_size, std::byte{0});
                for (quxlang::class_field_info const& field : layout_iter->second.fields)
                {
                    std::map< std::string, quxlang::antestatal_value >::const_iterator field_iter = struct_value.fields.find(field.name);
                    if (field_iter == struct_value.fields.end())
                    {
                        throw quxlang::semantic_compilation_error("Missing field '" + field.name + "' in readonly antestatal constant for " + quxlang::to_string(type));
                    }

                    std::vector< std::byte > const field_bytes = materialize_antestatal_bytes(field.type, field_iter->second);
                    if (field.offset + field_bytes.size() > result.size())
                    {
                        throw quxlang::semantic_compilation_error("Readonly antestatal field exceeds declared storage size for " + quxlang::to_string(type));
                    }
                    std::copy(field_bytes.begin(), field_bytes.end(), result.begin() + static_cast< std::ptrdiff_t >(field.offset));
                }
                return result;
            }

            throw quxlang::semantic_compilation_error("Unsupported readonly antestatal aggregate initializer for LLVM lowering: " + quxlang::to_string(type));
        }

        /**
         * Lowers an antestatal access used by a constant pointer value.
         */
        auto constant_pointer_from_antestatal_access(quxlang::antestatal_access const& access, quxlang::type_symbol const& target_type) -> llvm::Constant*
        {
            return rpnx::apply_visitor< llvm::Constant* >(
                access,
                [&](auto const& concrete_access) -> llvm::Constant*
                {
                    using access_type = std::decay_t< decltype(concrete_access) >;

                    if constexpr (std::is_same_v< access_type, quxlang::antestatal_nullptr >)
                    {
                        return llvm::ConstantPointerNull::get(opaque_pointer_type());
                    }
                    else if constexpr (std::is_same_v< access_type, quxlang::antestatal_access_global >)
                    {
                        if (target_type.type_is< quxlang::procedure_type >())
                        {
                            callable_abi abi = callable_abi_from_signature(target_type.get_as< quxlang::procedure_type >().signature);
                            llvm::Function* callee = get_or_create_external_function(concrete_access.symbol, abi);
                            return llvm::ConstantExpr::getPointerCast(callee, opaque_pointer_type());
                        }

                        llvm::GlobalVariable* global = get_or_create_constant_global(concrete_access.symbol, target_type);
                        return llvm::ConstantExpr::getPointerCast(global, opaque_pointer_type());
                    }
                    else
                    {
                        throw quxlang::semantic_compilation_error(
                            "Nested antestatal pointer access requires declared readonly global type inventory for LLVM lowering");
                    }
                });
        }

        /**
         * Creates an LLVM constant initializer for readonly antestatal data already carried in the packet.
         */
        auto create_antestatal_constant_initializer(quxlang::type_symbol const& type, quxlang::antestatal_value const& value) -> llvm::Constant*
        {
            if (type.type_is< quxlang::attached_type_reference >())
            {
                quxlang::attached_type_reference const& attached = type.get_as< quxlang::attached_type_reference >();
                if (attached.carrying_type.type_is< quxlang::void_type >())
                {
                    return llvm::Constant::getNullValue(value_storage_type(type));
                }
                return create_antestatal_constant_initializer(attached.carrying_type, value);
            }

            if (type.type_is< quxlang::bool_type >())
            {
                if (!value.type_is< quxlang::antestatal_primitive >())
                {
                    throw quxlang::semantic_compilation_error("Expected primitive readonly antestatal bool initializer");
                }
                std::vector< std::byte > const bytes = normalized_raw_bytes(value.get_as< quxlang::antestatal_primitive >().value, 1);
                return llvm::ConstantInt::get(bool_storage_type(), std::to_integer< std::uint8_t >(bytes.front()) == 0 ? 0 : 1);
            }

            if (type.type_is< quxlang::byte_type >())
            {
                if (!value.type_is< quxlang::antestatal_primitive >())
                {
                    throw quxlang::semantic_compilation_error("Expected primitive readonly antestatal byte initializer");
                }
                std::vector< std::byte > const bytes = normalized_raw_bytes(value.get_as< quxlang::antestatal_primitive >().value, 1);
                return llvm::ConstantInt::get(i8_type(), std::to_integer< std::uint8_t >(bytes.front()));
            }

            if (type.type_is< quxlang::int_type >())
            {
                if (!value.type_is< quxlang::antestatal_primitive >())
                {
                    throw quxlang::semantic_compilation_error("Expected primitive readonly antestatal integer initializer");
                }
                quxlang::int_type const& int_info = type.get_as< quxlang::int_type >();
                llvm::APInt const integer_value = little_endian_apint(value.get_as< quxlang::antestatal_primitive >().value, int_info.bits);
                return llvm::ConstantInt::get(context, integer_value);
            }

            if (type.type_is< quxlang::size_type >())
            {
                if (!value.type_is< quxlang::antestatal_primitive >())
                {
                    throw quxlang::semantic_compilation_error("Expected primitive readonly antestatal size initializer");
                }
                unsigned const bit_width = static_cast< unsigned >(input.machine_target.machine.pointer_size_bytes() * 8);
                llvm::APInt const integer_value = little_endian_apint(value.get_as< quxlang::antestatal_primitive >().value, bit_width);
                return llvm::ConstantInt::get(context, integer_value);
            }

            if (std::optional< unsigned > const nominal_bits = nominal_integer_bit_width(type); nominal_bits.has_value())
            {
                if (!value.type_is< quxlang::antestatal_primitive >())
                {
                    throw quxlang::semantic_compilation_error("Expected primitive readonly antestatal nominal integer initializer");
                }
                llvm::APInt const integer_value = little_endian_apint(value.get_as< quxlang::antestatal_primitive >().value, *nominal_bits);
                return llvm::ConstantInt::get(context, integer_value);
            }

            if (type.type_is< quxlang::float_type >())
            {
                if (!value.type_is< quxlang::antestatal_primitive >())
                {
                    throw quxlang::semantic_compilation_error("Expected primitive readonly antestatal float initializer");
                }

                llvm::Type* llvm_type = value_storage_type(type);
                unsigned const bit_width = llvm_type->getPrimitiveSizeInBits();
                llvm::APInt const float_bits = little_endian_apint(value.get_as< quxlang::antestatal_primitive >().value, bit_width);
                llvm::Constant* bit_pattern = llvm::ConstantInt::get(context, float_bits);
                return llvm::ConstantExpr::getBitCast(bit_pattern, llvm_type);
            }

            if (interface_runtime_type(type))
            {
                if (!value.type_is< quxlang::antestatal_interface >())
                {
                    throw quxlang::semantic_compilation_error("Expected interface readonly antestatal initializer for " + quxlang::to_string(type));
                }

                quxlang::antestatal_interface const& interface_value = value.get_as< quxlang::antestatal_interface >();
                llvm::Constant* interface_constant = create_private_interface_constant(interface_value.interface_type, interface_value.functions, interface_value.is_default);
                llvm::GlobalVariable* global = new llvm::GlobalVariable(
                    *module,
                    interface_constant->getType(),
                    true,
                    llvm::GlobalValue::PrivateLinkage,
                    interface_constant,
                    quxlang::to_string(interface_value.interface_type) + "$iface$const$" + std::to_string(helper_counter++));
                return llvm::ConstantExpr::getPointerCast(global, opaque_pointer_type());
            }

            if (type.type_is< quxlang::procedure_type >())
            {
                if (!value.type_is< quxlang::antestatal_ptrref >())
                {
                    throw quxlang::semantic_compilation_error("Expected procedure pointer readonly antestatal initializer");
                }
                return constant_pointer_from_antestatal_access(value.get_as< quxlang::antestatal_ptrref >().target, type);
            }

            if (quxlang::is_ptr(type) || quxlang::is_ref(type))
            {
                if (!value.type_is< quxlang::antestatal_ptrref >())
                {
                    throw quxlang::semantic_compilation_error("Expected pointer readonly antestatal initializer for " + quxlang::to_string(type));
                }
                return constant_pointer_from_antestatal_access(value.get_as< quxlang::antestatal_ptrref >().target, quxlang::remove_ptr(quxlang::remove_ref(type)));
            }
            if (type.type_is< quxlang::readonly_constant >())
            {
                if (!value.type_is< quxlang::antestatal_primitive >())
                {
                    throw quxlang::semantic_compilation_error("Expected primitive readonly constant initializer bytes");
                }

                std::vector< std::byte > const& bytes = value.get_as< quxlang::antestatal_primitive >().value;
                llvm::GlobalVariable* payload = create_private_constant_bytes_global(bytes, quxlang::to_string(input.target_name));
                llvm::Constant* zero = llvm::ConstantInt::get(i64_type(), 0);
                llvm::Constant* start_pointer = llvm::ConstantExpr::getInBoundsGetElementPtr(
                    payload->getValueType(),
                    payload,
                    llvm::ArrayRef< llvm::Constant* >{zero, zero});
                llvm::Constant* end_pointer = start_pointer;
                if (!bytes.empty())
                {
                    end_pointer = llvm::ConstantExpr::getInBoundsGetElementPtr(
                        i8_type(),
                        start_pointer,
                        llvm::ArrayRef< llvm::Constant* >{llvm::ConstantInt::get(i64_type(), bytes.size())});
                }

                return llvm::ConstantStruct::get(
                    llvm::cast< llvm::StructType >(value_storage_type(type)),
                    {start_pointer, end_pointer});
            }

            std::vector< std::byte > const bytes = materialize_antestatal_bytes(type, value);
            return constant_byte_array(bytes);
        }

        /**
         * Converts a source string to the byte payload used by STRING_CONSTANT values.
         */
        auto runtime_string_bytes(std::string const& text) const -> std::vector< std::byte >
        {
            std::vector< std::byte > bytes;
            bytes.reserve(text.size());
            for (char const ch : text)
            {
                bytes.push_back(static_cast< std::byte >(static_cast< unsigned char >(ch)));
            }
            return bytes;
        }

        /**
         * Materializes a private immutable STRING_CONSTANT object for runtime support calls.
         */
        auto create_runtime_string_constant_initializer(std::string const& text) -> llvm::Constant*
        {
            quxlang::type_symbol const string_constant_type = quxlang::llvm_backend::runtime_string_constant_type();
            quxlang::antestatal_value const value = quxlang::antestatal_primitive{.value = runtime_string_bytes(text)};
            return create_antestatal_constant_initializer(string_constant_type, value);
        }

        /**
         * Materializes a private immutable STRING_CONSTANT object for runtime support calls.
         */
        auto create_private_runtime_string_constant(std::string const& text, std::string const& name_stem) -> llvm::GlobalVariable*
        {
            quxlang::type_symbol const string_constant_type = quxlang::llvm_backend::runtime_string_constant_type();
            llvm::Constant* const initializer = create_runtime_string_constant_initializer(text);
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(
                *module,
                initializer->getType(),
                true,
                llvm::GlobalValue::PrivateLinkage,
                initializer,
                name_stem + "$strconst$" + std::to_string(helper_counter++));
            global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            global->setAlignment(llvm::Align(slot_alignment(string_constant_type)));
            return global;
        }

        /**
         * Returns the LLVM value for one source-order MODULE(RUNTIME)::ASSERT_FAIL argument.
         */
        auto runtime_assert_fail_argument_value(quxlang::llvm_backend::runtime_assert_fail_call_arguments const& args, abi_parameter const& parameter) -> llvm::Value*
        {
            if (!parameter.name.has_value())
            {
                throw quxlang::semantic_compilation_error("Runtime ASSERT_FAIL argument must be named");
            }

            std::string const& name = *parameter.name;
            if (name == "expr")
            {
                llvm::GlobalVariable* const object = create_private_runtime_string_constant(args.expr, quxlang::to_string(input.target_name));
                return llvm::ConstantExpr::getPointerCast(object, opaque_pointer_type());
            }
            if (name == "file")
            {
                return llvm::ConstantInt::get(pointer_integer_type(), args.file);
            }
            if (name == "line")
            {
                return llvm::ConstantInt::get(pointer_integer_type(), args.line);
            }
            if (name == "column")
            {
                return llvm::ConstantInt::get(pointer_integer_type(), args.column);
            }
            if (name == "tag")
            {
                if (!args.tag.has_value())
                {
                    return llvm::ConstantPointerNull::get(opaque_pointer_type());
                }

                llvm::GlobalVariable* const object = create_private_runtime_string_constant(*args.tag, quxlang::to_string(input.target_name));
                return llvm::ConstantExpr::getPointerCast(object, opaque_pointer_type());
            }

            throw quxlang::semantic_compilation_error("Unknown Runtime ASSERT_FAIL argument: " + name);
        }

        /**
         * Returns MODULE(RUNTIME)::ASSERT_FAIL call operands in selected ABI order.
         */
        auto runtime_assert_fail_call_arguments(quxlang::llvm_backend::runtime_assert_fail_call_arguments const& args, callable_abi const& abi) -> std::vector< llvm::Value* >
        {
            std::vector< llvm::Value* > values;
            values.reserve(abi.llvm_param_source_indices.size());
            for (std::size_t const source_index : abi.llvm_param_source_indices)
            {
                values.push_back(runtime_assert_fail_argument_value(args, abi.source_ordered.at(source_index)));
            }
            return values;
        }

        auto get_or_create_global(quxlang::type_symbol const& symbol, llvm::Type* storage_type, bool is_constant) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >& globals = is_constant ? constant_globals : mutable_globals;
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = globals.find(symbol);
            if (existing != globals.end())
            {
                return existing->second;
            }

            llvm::GlobalVariable* global = new llvm::GlobalVariable(
                *module,
                storage_type,
                is_constant,
                llvm::GlobalValue::ExternalLinkage,
                nullptr,
                quxlang::to_string(symbol));
            globals[symbol] = global;
            return global;
        }

        auto global_init_type(quxlang::type_symbol const& symbol) const -> quxlang::initialization_type
        {
            std::map< quxlang::type_symbol, quxlang::initialization_type >::const_iterator iter = input.global_init_types.find(symbol);
            if (iter == input.global_init_types.end())
            {
                return quxlang::initialization_type::init_with_guard;
            }

            return iter->second;
        }

        auto get_or_create_zero_initialized_global(quxlang::type_symbol const& symbol, llvm::Type* storage_type) -> llvm::GlobalVariable*
        {
            llvm::GlobalVariable* global = get_or_create_global(symbol, storage_type, false);
            if (global->isDeclaration())
            {
                global->setInitializer(llvm::Constant::getNullValue(storage_type));
            }
            return global;
        }

        auto get_or_create_common_zero_initialized_global(quxlang::type_symbol const& symbol, llvm::Type* storage_type) -> llvm::GlobalVariable*
        {
            llvm::GlobalVariable* global = get_or_create_zero_initialized_global(symbol, storage_type);
            global->setLinkage(llvm::GlobalValue::CommonLinkage);
            return global;
        }

        /**
         * Returns the LLVM global symbol name used for the initguard protecting one global.
         */
        auto initguard_global_symbol_name(quxlang::type_symbol const& symbol) const -> std::string
        {
            return quxlang::to_string(quxlang::type_symbol(quxlang::subsymbol{
                .of = symbol,
                .name = "INITGUARD",
            }));
        }

        /** Applies the requested VMIR access class to an LLVM global declaration or definition. */
        void apply_access_class(llvm::GlobalVariable* global, quxlang::vmir2::access_class class_) const
        {
            switch (class_)
            {
            case quxlang::vmir2::access_class::global:
                return;
            case quxlang::vmir2::access_class::thread:
                global->setThreadLocalMode(llvm::GlobalValue::LocalExecTLSModel);
                return;
            }
            throw quxlang::compiler_bug("unknown object access class");
        }

        auto is_main_function_object_symbol(quxlang::type_symbol const& symbol) const -> bool
        {
            return symbol.type_is< quxlang::builtin_symbol >() && symbol.get_as< quxlang::builtin_symbol >().name == "MAIN_FUNCTION";
        }

        auto should_emit_main_function_object_target() const -> bool
        {
            return input.whole_module && input.whole_module_output_kind == quxlang::output_kind::executable;
        }

        auto should_emit_unit_test_objects() const -> bool
        {
            return input.whole_module && input.whole_module_output_kind == quxlang::output_kind::unit_test_suite;
        }

        auto get_or_create_main_function_object_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            if (!object_type.type_is< quxlang::procedure_type >())
            {
                throw quxlang::semantic_compilation_error("MAIN_FUNCTION object must have a PROCEDURE type");
            }

            llvm::Type* storage_type = value_storage_type(object_type);
            llvm::Constant* initializer = llvm::Constant::getNullValue(storage_type);
            llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::WeakAnyLinkage;

            if (should_emit_main_function_object_target())
            {
                llvm::Function* const main_function = declared_function(input.target_name);
                if (main_function->arg_size() != 0 || !main_function->getReturnType()->isIntegerTy(32))
                {
                    throw quxlang::semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + quxlang::to_string(input.target_name));
                }

                initializer = llvm::ConstantExpr::getPointerCast(main_function, storage_type);
                linkage = llvm::GlobalValue::ExternalLinkage;
            }

            llvm::GlobalVariable* global = new llvm::GlobalVariable(
                *module,
                storage_type,
                true,
                linkage,
                initializer,
                quxlang::to_string(symbol));
            constant_globals[symbol] = global;
            return global;
        }

        auto unit_test_object_linkage() const -> llvm::GlobalValue::LinkageTypes
        {
            if (should_emit_unit_test_objects())
            {
                return llvm::GlobalValue::ExternalLinkage;
            }
            return llvm::GlobalValue::WeakAnyLinkage;
        }

        auto create_unit_test_names_array_pointer() -> llvm::Constant*
        {
            if (!should_emit_unit_test_objects() || input.unit_tests.empty())
            {
                return llvm::ConstantPointerNull::get(opaque_pointer_type());
            }

            quxlang::type_symbol const string_constant_type = quxlang::llvm_backend::runtime_string_constant_type();
            llvm::Type* const element_type = value_storage_type(string_constant_type);
            std::vector< llvm::Constant* > entries;
            entries.reserve(input.unit_tests.size());
            for (quxlang::llvm_backend::unit_test_entry const& unit_test : input.unit_tests)
            {
                entries.push_back(create_runtime_string_constant_initializer(unit_test.name));
            }

            llvm::ArrayType* const array_type = llvm::ArrayType::get(element_type, entries.size());
            llvm::Constant* const initializer = llvm::ConstantArray::get(array_type, entries);
            llvm::GlobalVariable* const table = new llvm::GlobalVariable(
                *module,
                array_type,
                true,
                llvm::GlobalValue::PrivateLinkage,
                initializer,
                quxlang::to_string(input.target_name) + "$unit_test_names$" + std::to_string(helper_counter++));
            table->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            table->setAlignment(llvm::Align(slot_alignment(string_constant_type)));

            llvm::Constant* const zero = llvm::ConstantInt::get(i64_type(), 0);
            llvm::Constant* const first = llvm::ConstantExpr::getInBoundsGetElementPtr(
                array_type,
                table,
                llvm::ArrayRef< llvm::Constant* >{zero, zero});
            return llvm::ConstantExpr::getPointerCast(first, opaque_pointer_type());
        }

        auto create_unit_test_proc_array_pointer() -> llvm::Constant*
        {
            if (!should_emit_unit_test_objects() || input.unit_tests.empty())
            {
                return llvm::ConstantPointerNull::get(opaque_pointer_type());
            }

            std::vector< llvm::Constant* > entries;
            entries.reserve(input.unit_tests.size());
            for (quxlang::llvm_backend::unit_test_entry const& unit_test : input.unit_tests)
            {
                entries.push_back(llvm::ConstantExpr::getPointerCast(declared_function(unit_test.procedure_symbol), opaque_pointer_type()));
            }

            llvm::ArrayType* const array_type = llvm::ArrayType::get(opaque_pointer_type(), entries.size());
            llvm::Constant* const initializer = llvm::ConstantArray::get(array_type, entries);
            llvm::GlobalVariable* const table = new llvm::GlobalVariable(
                *module,
                array_type,
                true,
                llvm::GlobalValue::PrivateLinkage,
                initializer,
                quxlang::to_string(input.target_name) + "$unit_test_proc$" + std::to_string(helper_counter++));
            table->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            table->setAlignment(llvm::Align(input.machine_target.machine.pointer_align()));

            llvm::Constant* const zero = llvm::ConstantInt::get(i64_type(), 0);
            llvm::Constant* const first = llvm::ConstantExpr::getInBoundsGetElementPtr(
                array_type,
                table,
                llvm::ArrayRef< llvm::Constant* >{zero, zero});
            return llvm::ConstantExpr::getPointerCast(first, opaque_pointer_type());
        }

        auto get_or_create_unit_test_count_object_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            if (object_type != quxlang::llvm_backend::unit_test_count_object_type())
            {
                throw quxlang::semantic_compilation_error("UNIT_TEST_COUNT object must have type SZ");
            }

            llvm::Type* const storage_type = value_storage_type(object_type);
            llvm::Constant* const initializer = llvm::ConstantInt::get(
                llvm::cast< llvm::IntegerType >(storage_type),
                should_emit_unit_test_objects() ? input.unit_tests.size() : 0);
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(
                *module,
                storage_type,
                true,
                unit_test_object_linkage(),
                initializer,
                quxlang::to_string(symbol));
            constant_globals[symbol] = global;
            return global;
        }

        auto get_or_create_unit_test_names_object_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            if (object_type != quxlang::llvm_backend::unit_test_names_object_type())
            {
                throw quxlang::semantic_compilation_error("UNIT_TEST_NAMES object must have type CONST=>> STRING_CONSTANT");
            }

            llvm::Type* const storage_type = value_storage_type(object_type);
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(
                *module,
                storage_type,
                true,
                unit_test_object_linkage(),
                create_unit_test_names_array_pointer(),
                quxlang::to_string(symbol));
            constant_globals[symbol] = global;
            return global;
        }

        auto get_or_create_unit_test_proc_object_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            if (object_type != quxlang::llvm_backend::unit_test_proc_object_type())
            {
                throw quxlang::semantic_compilation_error("UNIT_TEST_PROC object must have type CONST=>> PROCEDURE()");
            }

            llvm::Type* const storage_type = value_storage_type(object_type);
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(
                *module,
                storage_type,
                true,
                unit_test_object_linkage(),
                create_unit_test_proc_array_pointer(),
                quxlang::to_string(symbol));
            constant_globals[symbol] = global;
            return global;
        }

        auto get_or_create_unit_test_object_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            if (quxlang::llvm_backend::is_unit_test_count_object_symbol(symbol))
            {
                return get_or_create_unit_test_count_object_global(symbol, object_type);
            }
            if (quxlang::llvm_backend::is_unit_test_names_object_symbol(symbol))
            {
                return get_or_create_unit_test_names_object_global(symbol, object_type);
            }
            if (quxlang::llvm_backend::is_unit_test_proc_object_symbol(symbol))
            {
                return get_or_create_unit_test_proc_object_global(symbol, object_type);
            }
            throw quxlang::compiler_bug("not a unit-test builtin object: " + quxlang::to_string(symbol));
        }

        void emit_object_reference_globals()
        {
            for (std::pair< quxlang::type_symbol const, quxlang::type_symbol > const& object_reference : input.object_reference_types)
            {
                if (quxlang::llvm_backend::is_unit_test_object_symbol(object_reference.first))
                {
                    (void)get_or_create_unit_test_object_global(object_reference.first, object_reference.second);
                    continue;
                }

                if (is_main_function_object_symbol(object_reference.first))
                {
                    (void)get_or_create_main_function_object_global(object_reference.first, object_reference.second);
                    continue;
                }

                if (input.antestatal_constants.contains(object_reference.first))
                {
                    (void)get_or_create_constant_global(object_reference.first, object_reference.second);
                    continue;
                }

                (void)get_or_create_common_zero_initialized_global(object_reference.first, value_storage_type(object_reference.second));
            }
        }

        /**
         * Creates one readonly antestatal global, using a linkonce initializer when the packet carries the constant value.
         */
        auto get_or_create_constant_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& target_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            llvm::Type* storage_type = value_storage_type(target_type);
            std::map< quxlang::type_symbol, quxlang::antestatal_value >::const_iterator constant_iter = input.antestatal_constants.find(symbol);
            llvm::Constant* initializer = nullptr;
            llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::ExternalLinkage;
            if (constant_iter != input.antestatal_constants.end())
            {
                initializer = create_antestatal_constant_initializer(target_type, constant_iter->second);
                linkage = llvm::GlobalValue::LinkOnceODRLinkage;
            }

            llvm::GlobalVariable* global = new llvm::GlobalVariable(
                *module,
                storage_type,
                true,
                linkage,
                initializer,
                quxlang::to_string(symbol));
            constant_globals[symbol] = global;
            return global;
        }

        auto get_or_create_initguard_global(quxlang::type_symbol const& symbol, quxlang::vmir2::access_class class_) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = initguard_globals.find(symbol);
            if (existing != initguard_globals.end())
            {
                apply_access_class(existing->second, class_);
                return existing->second;
            }

            llvm::GlobalVariable* global = new llvm::GlobalVariable(
                *module,
                value_storage_type(quxlang::initguard_type{}),
                false,
                llvm::GlobalValue::CommonLinkage,
                llvm::Constant::getNullValue(value_storage_type(quxlang::initguard_type{})),
                initguard_global_symbol_name(symbol));
            apply_access_class(global, class_);
            initguard_globals[symbol] = global;
            return global;
        }

        auto get_interface_slots(quxlang::type_symbol const& interface_type) -> std::vector< quxlang::interface_slot_key > const&
        {
            std::map< quxlang::type_symbol, std::vector< quxlang::interface_slot_key > >::const_iterator iter = input.interface_slots.find(interface_type);
            if (iter == input.interface_slots.end())
            {
                throw quxlang::semantic_compilation_error("Missing interface slot inventory for LLVM lowering: " + quxlang::to_string(interface_type));
            }
            return iter->second;
        }

        auto get_or_create_interface_struct(quxlang::type_symbol const& interface_type) -> llvm::StructType*
        {
            std::map< quxlang::type_symbol, llvm::StructType* >::const_iterator existing = interface_structs.find(interface_type);
            if (existing != interface_structs.end())
            {
                return existing->second;
            }

            std::vector< quxlang::interface_slot_key > const& slots = get_interface_slots(interface_type);
            std::vector< llvm::Type* > fields;
            fields.reserve(slots.size() + 1);
            fields.push_back(i8_type());
            for (std::size_t i = 0; i < slots.size(); ++i)
            {
                fields.push_back(opaque_pointer_type());
                interface_slot_indices[interface_type][slots[i]] = i + 1;
            }

            llvm::StructType* struct_type = llvm::StructType::create(context, fields, quxlang::to_string(interface_type) + "$interface");
            interface_structs[interface_type] = struct_type;
            return struct_type;
        }

        /**
         * Splits one source-relative path into LLVM debug-info directory and filename components.
         */
        auto debug_file_parts(std::string const& path_string) const -> std::pair< std::string, std::string >
        {
            std::filesystem::path const path(path_string);
            std::filesystem::path const filename = path.filename();
            std::filesystem::path const parent = path.parent_path();
            std::string directory = parent.empty() ? "." : parent.string();
            std::string filename_string = filename.empty() ? path_string : filename.string();
            if (filename_string.empty())
            {
                filename_string = "<unknown>";
            }
            return std::make_pair(std::move(directory), std::move(filename_string));
        }

        /**
         * Returns the fallback DIFile used when a specific VMIR location does not resolve to a source file entry.
         */
        auto default_debug_file() -> llvm::DIFile*
        {
            if (!debug_builder)
            {
                return nullptr;
            }

            if (input.source_index.has_value() && !input.source_index->get().files.empty())
            {
                return get_or_create_debug_file(input.source_index->get().files.begin()->first);
            }

            return debug_builder->createFile("<unknown>", ".");
        }

        /**
         * Returns the DIFile for one source-index file id, creating it on first use.
         */
        auto get_or_create_debug_file(std::uint64_t file_id) -> llvm::DIFile*
        {
            std::map< std::uint64_t, llvm::DIFile* >::const_iterator existing = debug_files.find(file_id);
            if (existing != debug_files.end())
            {
                return existing->second;
            }

            if (!debug_builder || !input.source_index.has_value())
            {
                return nullptr;
            }

            std::map< std::uint64_t, quxlang::vmir2::indexed_source_file >::const_iterator file_iter = input.source_index->get().files.find(file_id);
            if (file_iter == input.source_index->get().files.end())
            {
                return default_debug_file();
            }

            std::pair< std::string, std::string > const parts = debug_file_parts(file_iter->second.path());
            llvm::DIFile* const file = debug_builder->createFile(parts.second, parts.first);
            debug_files[file_id] = file;
            return file;
        }

        /**
         * Finds the first available VMIR source location in one routine for subprogram metadata.
         */
        auto routine_debug_location(quxlang::vmir2::functanoid_routine3 const& routine) const -> std::optional< quxlang::source_location >
        {
            for (quxlang::vmir2::executable_block const& block : routine.blocks)
            {
                for (quxlang::vmir2::vm_instruction const& instruction : block.instructions)
                {
                    std::optional< quxlang::source_location > const location = quxlang::vmir2::get_location(instruction);
                    if (location.has_value())
                    {
                        return location;
                    }
                }
                if (block.terminator.has_value())
                {
                    std::optional< quxlang::source_location > const location = quxlang::vmir2::get_location(*block.terminator);
                    if (location.has_value())
                    {
                        return location;
                    }
                }
            }
            return std::nullopt;
        }

        /**
         * Returns the LLVM debug subprogram for one defined routine, creating it on demand.
         */
        auto debug_subprogram(quxlang::type_symbol const& symbol, quxlang::vmir2::functanoid_routine3 const& routine) -> llvm::DISubprogram*
        {
            std::map< quxlang::type_symbol, llvm::DISubprogram* >::const_iterator existing = debug_subprograms.find(symbol);
            if (existing != debug_subprograms.end())
            {
                return existing->second;
            }

            if (!debug_builder || debug_compile_unit == nullptr)
            {
                return nullptr;
            }

            std::optional< quxlang::source_location > const location = routine_debug_location(routine);
            llvm::DIFile* file = default_debug_file();
            unsigned line = 1;
            unsigned scope_line = 1;
            if (location.has_value() && input.source_index.has_value())
            {
                file = get_or_create_debug_file(location->file_id);
                std::map< std::uint64_t, quxlang::vmir2::indexed_source_file >::const_iterator file_iter = input.source_index->get().files.find(location->file_id);
                if (file_iter != input.source_index->get().files.end())
                {
                    quxlang::vmir2::source_position const position = file_iter->second.position(location->begin_index);
                    line = static_cast< unsigned >(position.line);
                    scope_line = line;
                }
            }

            llvm::DISubroutineType* const subroutine_type = debug_builder->createSubroutineType(debug_builder->getOrCreateTypeArray({}));
            llvm::DISubprogram* const subprogram = debug_builder->createFunction(
                file,
                quxlang::to_string(symbol),
                quxlang::to_string(symbol),
                file,
                line,
                subroutine_type,
                scope_line,
                llvm::DINode::FlagZero,
                llvm::DISubprogram::SPFlagDefinition);
            debug_subprograms[symbol] = subprogram;
            return subprogram;
        }

        /**
         * Applies one VMIR source location to subsequent LLVM instructions emitted through the shared builder.
         */
        void apply_debug_location(function_codegen_state& state, std::optional< quxlang::source_location > const& location)
        {
            if (!debug_builder || state.function->getSubprogram() == nullptr)
            {
                builder.SetCurrentDebugLocation(llvm::DebugLoc());
                return;
            }

            if (!location.has_value() || !input.source_index.has_value())
            {
                builder.SetCurrentDebugLocation(llvm::DILocation::get(
                    context,
                    std::max< unsigned >(state.function->getSubprogram()->getLine(), 1),
                    1,
                    state.function->getSubprogram()));
                return;
            }

            std::map< std::uint64_t, quxlang::vmir2::indexed_source_file >::const_iterator file_iter = input.source_index->get().files.find(location->file_id);
            if (file_iter == input.source_index->get().files.end())
            {
                builder.SetCurrentDebugLocation(llvm::DILocation::get(
                    context,
                    std::max< unsigned >(state.function->getSubprogram()->getLine(), 1),
                    1,
                    state.function->getSubprogram()));
                return;
            }

            quxlang::vmir2::source_position const position = file_iter->second.position(location->begin_index);
            builder.SetCurrentDebugLocation(llvm::DILocation::get(
                context,
                static_cast< unsigned >(position.line),
                static_cast< unsigned >(position.column),
                state.function->getSubprogram()));
        }

        auto value_address(function_codegen_state& state, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            std::size_t const slot_index = local_slot_index(slot);
            local_slot_state& local_state = state.locals.at(slot_index);
            if (local_state.aliased_value_address != nullptr)
            {
                return local_state.aliased_value_address;
            }
            if (local_state.storage == nullptr)
            {
                throw quxlang::semantic_compilation_error("VMIR slot does not have runtime storage in LLVM lowering: %" + std::to_string(std::uint64_t(slot)));
            }
            return local_state.storage;
        }

        auto load_slot_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(slot)).type;
            llvm::Type* storage_type = value_storage_type(type);
            return ir_builder.CreateLoad(storage_type, value_address(state, slot));
        }

        void store_slot_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot, llvm::Value* value)
        {
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(slot)).type;
            if (quxlang::is_atomic_type(type))
            {
                value = logical_atomic_value_to_storage(ir_builder, type, value);
            }
            ir_builder.CreateStore(value, value_address(state, slot));
        }

        auto load_reference_pointer(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            return ir_builder.CreateLoad(opaque_pointer_type(), state.locals.at(local_slot_index(slot)).storage);
        }

        void store_reference_pointer(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot, llvm::Value* pointer_value)
        {
            ir_builder.CreateStore(pointer_value, state.locals.at(local_slot_index(slot)).storage);
        }

        auto output_argument_pointer(function_codegen_state& state, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            return value_address(state, slot);
        }

        void store_boolean(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot, llvm::Value* condition)
        {
            llvm::Value* bool_value = ir_builder.CreateZExt(condition, bool_storage_type());
            store_slot_value(state, ir_builder, slot, bool_value);
        }

        auto truth_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(slot)).type;
            llvm::Value* value = load_slot_value(state, ir_builder, slot);
            if (type.type_is< quxlang::bool_type >() || type.type_is< quxlang::byte_type >())
            {
                return ir_builder.CreateICmpNE(value, llvm::ConstantInt::get(bool_storage_type(), 0));
            }
            if (type.type_is< quxlang::int_type >() || type.type_is< quxlang::size_type >())
            {
                return ir_builder.CreateICmpNE(value, llvm::ConstantInt::get(llvm::cast< llvm::IntegerType >(value->getType()), 0));
            }
            if (nominal_integer_runtime_type(type))
            {
                return ir_builder.CreateICmpNE(value, llvm::ConstantInt::get(llvm::cast< llvm::IntegerType >(value->getType()), 0));
            }
            if (type.type_is< quxlang::float_type >())
            {
                return ir_builder.CreateFCmpONE(value, llvm::ConstantFP::get(value->getType(), 0.0));
            }
            if (is_pointer_valued_type(type))
            {
                return ir_builder.CreateICmpNE(value, llvm::ConstantPointerNull::get(opaque_pointer_type()));
            }
            throw quxlang::semantic_compilation_error("Cannot form boolean condition from type: " + quxlang::to_string(type));
        }

        auto slot_alignment(quxlang::type_symbol const& type) const -> std::uint64_t
        {
            std::map< quxlang::type_symbol, quxlang::type_placement_info >::const_iterator iter = input.type_placements.find(type);
            if (iter != input.type_placements.end())
            {
                return std::max< std::uint64_t >(iter->second.alignment, 1);
            }
            if (type.type_is< quxlang::array_initializer_type >())
            {
                return array_initializer_storage_placement().alignment;
            }
            if (type.type_is< quxlang::int_type >())
            {
                return std::max< std::uint64_t >(type.get_as< quxlang::int_type >().bits / 8, 1);
            }
            if (type.type_is< quxlang::float_type >())
            {
                return std::max< std::uint64_t >(type.get_as< quxlang::float_type >().bits / 8, 1);
            }
            if (type.type_is< quxlang::byte_type >() || type.type_is< quxlang::bool_type >())
            {
                return 1;
            }
            if (is_pointer_valued_type(type) || is_output_slot_type(type))
            {
                return input.machine_target.machine.pointer_align();
            }
            throw quxlang::semantic_compilation_error("Missing type placement for LLVM lowering alignment: " + quxlang::to_string(type));
        }

        auto slot_size(quxlang::type_symbol const& type) const -> std::uint64_t
        {
            std::map< quxlang::type_symbol, quxlang::type_placement_info >::const_iterator iter = input.type_placements.find(type);
            if (iter != input.type_placements.end())
            {
                return iter->second.size;
            }
            if (type.type_is< quxlang::array_initializer_type >())
            {
                return array_initializer_storage_placement().size;
            }
            if (type.type_is< quxlang::bool_type >() || type.type_is< quxlang::byte_type >())
            {
                return 1;
            }
            if (type.type_is< quxlang::int_type >())
            {
                return std::max< std::uint64_t >(type.get_as< quxlang::int_type >().bits / 8, 1);
            }
            if (type.type_is< quxlang::float_type >())
            {
                return std::max< std::uint64_t >(type.get_as< quxlang::float_type >().bits / 8, 1);
            }
            if (is_pointer_valued_type(type) || is_output_slot_type(type))
            {
                return input.machine_target.machine.pointer_size_bytes();
            }
            throw quxlang::semantic_compilation_error("Missing type placement for LLVM lowering size: " + quxlang::to_string(type));
        }

        auto integer_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(slot)).type;
            if (!(type.type_is< quxlang::int_type >() || type.type_is< quxlang::bool_type >() || type.type_is< quxlang::byte_type >() || type.type_is< quxlang::size_type >() ||
                  nominal_integer_runtime_type(type)))
            {
                throw quxlang::semantic_compilation_error("Expected integer-like slot for LLVM lowering: " + quxlang::to_string(type));
            }
            return load_slot_value(state, ir_builder, slot);
        }

        /**
         * Converts one integer bit pattern to the requested LLVM integer width without changing its low bits.
         */
        auto integer_bits_to_width(ir_builder_t& ir_builder, llvm::Value* value, llvm::IntegerType* destination_type) -> llvm::Value*
        {
            llvm::IntegerType* const source_type = llvm::cast< llvm::IntegerType >(value->getType());
            if (source_type == destination_type)
            {
                return value;
            }
            if (source_type->getBitWidth() > destination_type->getBitWidth())
            {
                return ir_builder.CreateTrunc(value, destination_type);
            }
            return ir_builder.CreateZExt(value, destination_type);
        }

        /**
         * Converts a logical atomic integer value to the widened LLVM storage width used for atomic memory operations.
         */
        auto logical_atomic_value_to_storage(ir_builder_t& ir_builder, quxlang::type_symbol const& atomic_type, llvm::Value* value) -> llvm::Value*
        {
            llvm::Type* llvm_storage_type = value_storage_type(atomic_type);
            if (std::optional< quxlang::type_symbol > const atomic_value_type = quxlang::atomic_type_argument(atomic_type); atomic_value_type.has_value())
            {
                if (!atomic_value_type->type_is< quxlang::int_type >())
                {
                    llvm_storage_type = value_storage_type(*atomic_value_type);
                }
            }
            if (value->getType() == llvm_storage_type)
            {
                return value;
            }
            if (!value->getType()->isIntegerTy() || !llvm_storage_type->isIntegerTy())
            {
                throw quxlang::semantic_compilation_error("Atomic storage coercion requires integer LLVM values");
            }
            return integer_bits_to_width(ir_builder, value, llvm::cast< llvm::IntegerType >(llvm_storage_type));
        }

        /**
         * Converts a widened LLVM atomic storage value back to the logical ATOMIC#T value width.
         */
        auto storage_atomic_value_to_logical(ir_builder_t& ir_builder, quxlang::type_symbol const& atomic_type, llvm::Value* value) -> llvm::Value*
        {
            quxlang::type_symbol const logical_type = quxlang::atomic_storage_type_or_self(atomic_type);
            llvm::Type* const llvm_logical_type = value_storage_type(logical_type);
            if (value->getType() == llvm_logical_type)
            {
                return value;
            }
            if (!value->getType()->isIntegerTy() || !llvm_logical_type->isIntegerTy())
            {
                throw quxlang::semantic_compilation_error("Atomic logical coercion requires integer LLVM values");
            }
            return integer_bits_to_width(ir_builder, value, llvm::cast< llvm::IntegerType >(llvm_logical_type));
        }

        auto scalar_one(llvm::Type* type) -> llvm::Constant*
        {
            if (llvm::IntegerType* integer_type = llvm::dyn_cast< llvm::IntegerType >(type))
            {
                return llvm::ConstantInt::get(integer_type, 1);
            }
            if (type->isFloatTy() || type->isDoubleTy() || type->isHalfTy() || type->isX86_FP80Ty() || type->isFP128Ty())
            {
                return llvm::ConstantFP::get(type, 1.0);
            }
            throw quxlang::semantic_compilation_error("Cannot construct scalar one for LLVM type");
        }

        auto scalar_zero(llvm::Type* type) -> llvm::Constant*
        {
            return llvm::Constant::getNullValue(type);
        }

        /**
         * Returns the address of a field within a runtime aggregate at an already-known base pointer.
         */
        auto field_address_from_base_pointer(
            function_codegen_state& state,
            ir_builder_t& ir_builder,
            llvm::Value* base_pointer,
            quxlang::type_symbol base_type,
            std::string const& field_name,
            quxlang::type_symbol const& field_type) -> llvm::Value*
        {
            if (quxlang::is_ref(base_type))
            {
                base_type = quxlang::remove_ref(base_type);
            }
            if (quxlang::is_ptr(base_type))
            {
                base_type = quxlang::remove_ptr(base_type);
            }

            std::map< quxlang::type_symbol, quxlang::class_layout >::const_iterator layout_iter = input.class_layouts.find(base_type);
            if (layout_iter == input.class_layouts.end())
            {
                throw quxlang::semantic_compilation_error("Missing class layout for LLVM lowering: " + quxlang::to_string(base_type));
            }

            for (quxlang::class_field_info const& field : layout_iter->second.fields)
            {
                if (field.name == field_name)
                {
                    llvm::Value* byte_pointer = ir_builder.CreateBitCast(base_pointer, opaque_pointer_type());
                    llvm::Value* offset_pointer = ir_builder.CreateInBoundsGEP(i8_type(), byte_pointer, llvm::ConstantInt::get(i64_type(), field.offset));
                    return ir_builder.CreateBitCast(offset_pointer, llvm::PointerType::get(context, 0));
                }
            }

            throw quxlang::semantic_compilation_error("Unknown field '" + field_name + "' in layout for " + quxlang::to_string(base_type));
        }

        /**
         * Returns the address of a field within the object referenced by a VMIR reference slot.
         */
        auto referenced_field_address(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index base_slot, std::string const& field_name, quxlang::type_symbol const& field_type) -> llvm::Value*
        {
            quxlang::type_symbol const& base_type = state.routine->local_types.at(local_slot_index(base_slot)).type;
            return field_address_from_base_pointer(state, ir_builder, load_reference_pointer(state, ir_builder, base_slot), base_type, field_name, field_type);
        }

        /**
         * Returns the address of a field within a VMIR value slot's own stack storage.
         */
        auto stored_value_field_address(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index base_slot, std::string const& field_name, quxlang::type_symbol const& field_type) -> llvm::Value*
        {
            quxlang::type_symbol const& base_type = state.routine->local_types.at(local_slot_index(base_slot)).type;
            return field_address_from_base_pointer(state, ir_builder, value_address(state, base_slot), base_type, field_name, field_type);
        }

        /**
         * Lowers one INITVAL into the canonical readonly constant {__start,__end} runtime layout.
         */
        void store_readonly_constant_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index target, std::vector< std::byte > const& value)
        {
            quxlang::type_symbol const& target_type = state.routine->local_types.at(local_slot_index(target)).type;
            if (!target_type.type_is< quxlang::readonly_constant >())
            {
                throw quxlang::semantic_compilation_error("INITVAL currently requires a readonly constant target, got: " + quxlang::to_string(target_type));
            }

            quxlang::type_symbol const byte_pointer_type = quxlang::ptrref_type{
                .target = quxlang::byte_type{},
                .ptr_class = quxlang::pointer_class::array,
                .qual = quxlang::qualifier::constant,
            };

            llvm::GlobalVariable* payload = create_private_constant_bytes_global(value, quxlang::to_string(input.target_name));
            llvm::Value* start_pointer = ir_builder.CreateInBoundsGEP(
                payload->getValueType(),
                payload,
                {llvm::ConstantInt::get(i64_type(), 0), llvm::ConstantInt::get(i64_type(), 0)});
            llvm::Value* end_pointer = start_pointer;
            if (!value.empty())
            {
                end_pointer = ir_builder.CreateInBoundsGEP(i8_type(), start_pointer, llvm::ConstantInt::get(i64_type(), value.size()));
            }

            ir_builder.CreateStore(start_pointer, stored_value_field_address(state, ir_builder, target, "__start", byte_pointer_type));
            ir_builder.CreateStore(end_pointer, stored_value_field_address(state, ir_builder, target, "__end", byte_pointer_type));
        }

        auto direct_callee_abi(quxlang::type_symbol const& callee, quxlang::vmir2::invoke const& call, function_codegen_state const& state) -> callable_abi
        {
            std::map< quxlang::type_symbol, callable_abi >::const_iterator abi_iter = function_abis.find(callee);
            if (abi_iter != function_abis.end())
            {
                return abi_iter->second;
            }
            if (callee.type_is< quxlang::instanciation_reference >())
            {
                std::optional< quxlang::type_symbol > return_slot_type;
                std::map< std::string, quxlang::vmir2::local_index >::const_iterator return_iter = call.args.named.find("RETURN");
                if (return_iter != call.args.named.end())
                {
                    return_slot_type = state.routine->local_types.at(local_slot_index(return_iter->second)).type;
                }
                return callable_abi_from_instanciation_reference(callee.get_as< quxlang::instanciation_reference >(), return_slot_type);
            }
            return callable_abi_from_invoke(call, state);
        }

        auto direct_callee_abi(quxlang::type_symbol const& callee, quxlang::vmir2::invocation_args const& args, function_codegen_state const& state) -> callable_abi
        {
            std::map< quxlang::type_symbol, callable_abi >::const_iterator abi_iter = function_abis.find(callee);
            if (abi_iter != function_abis.end())
            {
                return abi_iter->second;
            }
            if (callee.type_is< quxlang::instanciation_reference >())
            {
                std::optional< quxlang::type_symbol > return_slot_type;
                std::map< std::string, quxlang::vmir2::local_index >::const_iterator return_iter = args.named.find("RETURN");
                if (return_iter != args.named.end())
                {
                    return_slot_type = state.routine->local_types.at(local_slot_index(return_iter->second)).type;
                }
                return callable_abi_from_instanciation_reference(callee.get_as< quxlang::instanciation_reference >(), return_slot_type);
            }

            throw quxlang::semantic_compilation_error("Cannot infer LLVM ABI for callable symbol: " + quxlang::to_string(callee));
        }

        auto interface_slot_abi(quxlang::interface_slot_key const& slot) -> callable_abi
        {
            quxlang::sigtype signature{
                .params = slot.concrete_params,
                .return_type = slot.concrete_return_type,
            };
            return callable_abi_from_signature(signature);
        }

        /**
         * Resolves one source-order callable parameter to its VMIR argument slot.
         */
        auto source_argument_slot(callable_abi const& abi, quxlang::vmir2::invocation_args const& args, std::size_t source_index) const -> quxlang::vmir2::local_index
        {
            abi_parameter const& source_param = abi.source_ordered.at(source_index);
            if (source_param.positional_index.has_value())
            {
                if (*source_param.positional_index >= args.positional.size())
                {
                    throw quxlang::semantic_compilation_error("Missing positional LLVM call argument");
                }
                return args.positional.at(*source_param.positional_index);
            }

            if (!source_param.name.has_value())
            {
                throw quxlang::semantic_compilation_error("Missing source parameter name for LLVM lowering");
            }

            auto const arg_iter = args.named.find(*source_param.name);
            if (arg_iter == args.named.end())
            {
                std::string available_named;
                bool first = true;
                for (std::pair< std::string const, quxlang::vmir2::local_index > const& arg : args.named)
                {
                    if (!first)
                    {
                        available_named += ", ";
                    }
                    first = false;
                    available_named += arg.first;
                }
                throw quxlang::semantic_compilation_error(
                    "Missing named LLVM call argument '" + *source_param.name + "'"
                    + " while resolving source index " + std::to_string(source_index)
                    + " of ABI parameter type " + quxlang::to_string(source_param.type)
                    + "; available named args: [" + available_named + "]"
                    + "; positional arg count: " + std::to_string(args.positional.size()));
            }
            return arg_iter->second;
        }

        /**
         * Returns the destination slot that receives the LLVM return value for one VMIR call.
         */
        auto call_return_slot(callable_abi const& abi, quxlang::vmir2::invocation_args const& args) const -> std::optional< quxlang::vmir2::local_index >
        {
            if (!abi.return_source_index.has_value())
            {
                return std::nullopt;
            }
            return source_argument_slot(abi, args, *abi.return_source_index);
        }

        auto call_argument_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index arg_slot, quxlang::type_symbol const& param_type) -> llvm::Value*
        {
            if (is_output_slot_type(param_type))
            {
                return output_argument_pointer(state, arg_slot);
            }

            if (!abi_passes_by_value(param_type))
            {
                return value_address(state, arg_slot);
            }
            return load_slot_value(state, ir_builder, arg_slot);
        }

        auto ordered_call_arguments(function_codegen_state& state, ir_builder_t& ir_builder, callable_abi const& abi, quxlang::vmir2::invocation_args const& args) -> std::vector< llvm::Value* >
        {
            std::vector< llvm::Value* > values;
            values.reserve(abi.llvm_param_source_indices.size());
            for (std::size_t const source_index : abi.llvm_param_source_indices)
            {
                quxlang::vmir2::local_index const arg_slot = source_argument_slot(abi, args, source_index);
                values.push_back(call_argument_value(state, ir_builder, arg_slot, abi.source_ordered.at(source_index).type));
            }
            return values;
        }

        /**
         * Returns the declared parameter type for one routine parameter local, if the slot is a formal parameter.
         */
        auto routine_parameter_type(function_codegen_state const& state, quxlang::vmir2::local_index slot) const -> std::optional< quxlang::type_symbol >
        {
            for (quxlang::vmir2::routine_parameter const& param : state.routine->parameters.positional)
            {
                if (param.local_index == slot)
                {
                    return param.type;
                }
            }
            for (std::pair< std::string const, quxlang::vmir2::routine_parameter > const& param : state.routine->parameters.named)
            {
                if (param.second.local_index == slot)
                {
                    return param.second.type;
                }
            }
            return std::nullopt;
        }

        /**
         * Rebuilds the VMIR argument map for the current routine parameter list.
         */
        auto routine_parameter_invocation_args(function_codegen_state const& state) const -> quxlang::vmir2::invocation_args
        {
            quxlang::vmir2::invocation_args args;
            args.positional.reserve(state.routine->parameters.positional.size());
            for (quxlang::vmir2::routine_parameter const& param : state.routine->parameters.positional)
            {
                args.positional.push_back(param.local_index);
            }
            for (std::pair< std::string const, quxlang::vmir2::routine_parameter > const& param : state.routine->parameters.named)
            {
                args.named[param.first] = param.second.local_index;
            }
            if (state.abi != nullptr && state.abi->return_source_index.has_value())
            {
                std::size_t const return_source_index = *state.abi->return_source_index;
                if (return_source_index < state.abi->source_ordered.size())
                {
                    abi_parameter const& return_param = state.abi->source_ordered.at(return_source_index);
                    if (return_param.name.has_value())
                    {
                        std::vector< routine_abi_parameter > const ordered_params = ordered_routine_parameters(*state.routine);
                        if (return_source_index < ordered_params.size())
                        {
                            args.named[*return_param.name] = ordered_params.at(return_source_index).local;
                        }
                    }
                }
            }
            return args;
        }

        auto invocation_args_with_this(quxlang::vmir2::invocation_args args, quxlang::vmir2::local_index this_slot) const -> quxlang::vmir2::invocation_args
        {
            args.named["THIS"] = this_slot;
            return args;
        }

        /**
         * Returns true when the slot exits the current lifetime by transferring ownership to a caller-visible slot.
         */
        auto is_return_transfer_slot(function_codegen_state const& state, quxlang::vmir2::local_index slot) const -> bool
        {
            std::optional< quxlang::type_symbol > const parameter_type = routine_parameter_type(state, slot);
            if (!parameter_type.has_value())
            {
                return false;
            }
            return is_output_slot_type(*parameter_type);
        }

        /**
         * Returns true when this VMIR slot is only an alias/delegate view over some other storage owner.
         */
        auto is_cleanup_alias(quxlang::vmir2::slot_state const& slot_state) const -> bool
        {
            return slot_state.delegate_of.has_value() || slot_state.array_delegate_of_initializer.has_value() || slot_state.destroy_delegate;
        }

        /**
         * Stores LLVM poison into the storage region backing one VMIR slot.
         */
        void poison_slot_storage(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot)
        {
            quxlang::type_symbol const& slot_type = state.routine->local_types.at(local_slot_index(slot)).type;
            ir_builder.CreateStore(llvm::PoisonValue::get(value_storage_type(slot_type)), value_address(state, slot));
        }

        /**
         * Emits a direct helper call to the destructor associated with one live VMIR slot.
         */
        void emit_slot_destructor_call(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot)
        {
            quxlang::type_symbol const& slot_type = state.routine->local_types.at(local_slot_index(slot)).type;
            std::map< quxlang::type_symbol, quxlang::type_symbol >::const_iterator dtor_iter = state.routine->non_trivial_dtors.find(slot_type);
            if (dtor_iter == state.routine->non_trivial_dtors.end())
            {
                return;
            }

            quxlang::type_symbol const& dtor_symbol = dtor_iter->second;
            callable_abi abi;
            std::map< quxlang::type_symbol, callable_abi >::const_iterator abi_iter = function_abis.find(dtor_symbol);
            if (abi_iter != function_abis.end())
            {
                abi = abi_iter->second;
            }
            else if (dtor_symbol.type_is< quxlang::instanciation_reference >())
            {
                abi = callable_abi_from_instanciation_reference(dtor_symbol.get_as< quxlang::instanciation_reference >(), std::nullopt);
            }
            else
            {
                throw quxlang::semantic_compilation_error("Cannot infer LLVM ABI for destructor helper: " + quxlang::to_string(dtor_symbol));
            }

            quxlang::vmir2::invocation_args args;
            args.named["THIS"] = slot;
            llvm::Function* callee = get_or_create_external_function(dtor_symbol, abi);
            apply_calling_convention(ir_builder.CreateCall(abi.llvm_type, callee, ordered_call_arguments(state, ir_builder, abi, args)), abi);
        }

        /**
         * Emits an initguard complete or abort runtime call for a live lock slot.
         */
        void emit_initguard_runtime_call(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot, bool abort_lock)
        {
            llvm::Value* lock_value = load_slot_value(state, ir_builder, slot);
            quxlang::llvm_backend::runtime_procedure const procedure = abort_lock
                                                                            ? quxlang::llvm_backend::runtime_procedure::initguard_abort
                                                                            : quxlang::llvm_backend::runtime_procedure::initguard_complete;
            callable_abi const abi = initguard_runtime_abi(procedure);
            llvm::Function* callee = get_or_create_initguard_runtime_function(procedure, abi);
            llvm::CallInst* call = ir_builder.CreateCall(abi.llvm_type, callee, {lock_value});
            apply_calling_convention(call, abi);
        }

        /**
         * Returns true when a disappearing live slot needs runtime cleanup before leaving the current edge.
         */
        auto slot_requires_edge_cleanup(function_codegen_state const& state, quxlang::vmir2::local_index slot, quxlang::vmir2::slot_state const& slot_state) const -> bool
        {
            if (!slot_state.alive())
            {
                return false;
            }

            quxlang::type_symbol const& slot_type = state.routine->local_types.at(local_slot_index(slot)).type;
            if (slot_type.type_is< quxlang::initguard_lock_type >())
            {
                return true;
            }
            if (!slot_state.dtor_enabled() || is_cleanup_alias(slot_state))
            {
                return false;
            }
            return state.routine->non_trivial_dtors.contains(slot_type);
        }

        /**
         * Emits cleanup calls and storage poisoning for live locals that do not survive into the successor state.
         */
        void emit_transition_cleanup(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::state_map const& current_state, quxlang::vmir2::state_map const& target_state)
        {
            for (std::pair< quxlang::vmir2::local_index const, quxlang::vmir2::slot_state > const& slot_entry : current_state)
            {
                quxlang::vmir2::local_index const slot = slot_entry.first;
                quxlang::vmir2::slot_state const& slot_state = slot_entry.second;
                bool const alive_in_target = target_state.contains(slot) && target_state.at(slot).alive();
                if (alive_in_target || !slot_requires_edge_cleanup(state, slot, slot_state))
                {
                    if (alive_in_target || !slot_state.alive() || is_cleanup_alias(slot_state))
                    {
                        continue;
                    }
                }

                if (alive_in_target || !slot_state.alive())
                {
                    continue;
                }

                quxlang::type_symbol const& slot_type = state.routine->local_types.at(local_slot_index(slot)).type;
                if (slot_type.type_is< quxlang::initguard_lock_type >())
                {
                    emit_initguard_runtime_call(state, ir_builder, slot, true);
                }
                else if (!is_cleanup_alias(slot_state))
                {
                    emit_slot_destructor_call(state, ir_builder, slot);
                }

                if (!is_cleanup_alias(slot_state))
                {
                    poison_slot_storage(state, ir_builder, slot);
                }
            }
        }

        /**
         * Advances an array initializer after its current element alias transitions from dead to alive.
         */
        void emit_post_instruction_array_initializer_progress(
            function_codegen_state& state,
            ir_builder_t& ir_builder,
            quxlang::vmir2::state_map const& previous_state,
            quxlang::vmir2::state_map const& current_state)
        {
            for (std::pair< quxlang::vmir2::local_index const, quxlang::vmir2::slot_state > const& slot_entry : current_state)
            {
                quxlang::vmir2::local_index const slot = slot_entry.first;
                quxlang::vmir2::slot_state const& current_slot_state = slot_entry.second;
                if (!current_slot_state.array_delegate_of_initializer.has_value() || !current_slot_state.alive())
                {
                    continue;
                }

                std::map< quxlang::vmir2::local_index, quxlang::vmir2::slot_state >::const_iterator previous_iter = previous_state.find(slot);
                if (previous_iter == previous_state.end() || previous_iter->second.alive())
                {
                    continue;
                }

                quxlang::vmir2::local_index const initializer_slot = *current_slot_state.array_delegate_of_initializer;
                quxlang::type_symbol const initializer_type = state.routine->local_types.at(local_slot_index(initializer_slot)).type;
                llvm::Value* initializer_storage = state.locals.at(local_slot_index(initializer_slot)).storage;
                llvm::Value* index_field = ir_builder.CreateStructGEP(value_storage_type(initializer_type), initializer_storage, 1);
                llvm::Value* index_value = ir_builder.CreateLoad(i64_type(), index_field);
                ir_builder.CreateStore(ir_builder.CreateAdd(index_value, llvm::ConstantInt::get(i64_type(), 1)), index_field);
            }
        }

        /**
         * Poisons storage for ordinary in-block lifetime transitions after an instruction consumes a slot.
         * This excludes explicit destroy/end_lifetime, which emit their own storage cleanup directly.
         */
        void emit_post_instruction_poison_cleanup(
            function_codegen_state& state,
            ir_builder_t& ir_builder,
            quxlang::vmir2::state_map const& previous_state,
            quxlang::vmir2::state_map const& current_state,
            quxlang::vmir2::vm_instruction const& instruction)
        {
            if (instruction.type_is< quxlang::vmir2::destroy >() || instruction.type_is< quxlang::vmir2::end_lifetime >())
            {
                return;
            }

            for (std::pair< quxlang::vmir2::local_index const, quxlang::vmir2::slot_state > const& slot_entry : previous_state)
            {
                quxlang::vmir2::local_index const slot = slot_entry.first;
                quxlang::vmir2::slot_state const& previous_slot_state = slot_entry.second;
                bool const alive_in_current = current_state.contains(slot) && current_state.at(slot).alive();
                if (!previous_slot_state.alive() || alive_in_current || is_cleanup_alias(previous_slot_state))
                {
                    continue;
                }

                poison_slot_storage(state, ir_builder, slot);
            }
        }

        /**
         * Emits cleanup calls required before a normal return by transitioning into the state-engine normal-exit state.
         */
        void emit_return_cleanup(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::state_map const& current_state)
        {
            quxlang::vmir2::state_map exit_state;
            quxlang::vmir2::codegen_state_engine state_engine(exit_state, state.routine->local_types, state.routine->parameters);
            state_engine.apply_normal_exit();
            for (std::pair< quxlang::vmir2::local_index const, quxlang::vmir2::slot_state > const& slot_entry : current_state)
            {
                quxlang::vmir2::local_index const slot = slot_entry.first;
                quxlang::vmir2::slot_state const& slot_state = slot_entry.second;
                bool const alive_in_target = exit_state.contains(slot) && exit_state.at(slot).alive();
                if (alive_in_target || !slot_requires_edge_cleanup(state, slot, slot_state))
                {
                    if (alive_in_target || !slot_state.alive() || is_cleanup_alias(slot_state))
                    {
                        continue;
                    }
                }

                if (alive_in_target || !slot_state.alive())
                {
                    continue;
                }

                quxlang::type_symbol const& slot_type = state.routine->local_types.at(local_slot_index(slot)).type;
                if (slot_type.type_is< quxlang::initguard_lock_type >())
                {
                    emit_initguard_runtime_call(state, ir_builder, slot, true);
                }
                else if (!is_cleanup_alias(slot_state))
                {
                    std::optional< quxlang::type_symbol > const parameter_type = routine_parameter_type(state, slot);
                    if (!parameter_type.has_value() || !parameter_type->type_is< quxlang::dvalue_slot >())
                    {
                        emit_slot_destructor_call(state, ir_builder, slot);
                    }
                }

                if (!is_cleanup_alias(slot_state))
                {
                    poison_slot_storage(state, ir_builder, slot);
                }
            }
        }

        /**
         * Returns true when an edge needs a dedicated cleanup block before reaching the requested successor state.
         */
        auto edge_needs_cleanup(function_codegen_state const& state, quxlang::vmir2::state_map const& current_state, quxlang::vmir2::state_map const& target_state) const -> bool
        {
            for (std::pair< quxlang::vmir2::local_index const, quxlang::vmir2::slot_state > const& slot_entry : current_state)
            {
                bool const alive_in_target = target_state.contains(slot_entry.first) && target_state.at(slot_entry.first).alive();
                if (!alive_in_target && slot_entry.second.alive() && !is_cleanup_alias(slot_entry.second))
                {
                    return true;
                }
                if (!alive_in_target && slot_requires_edge_cleanup(state, slot_entry.first, slot_entry.second))
                {
                    return true;
                }
            }
            return false;
        }

        /**
         * Returns a transition block name of the form <src>.transition.<dest>,
         * adding a numeric suffix only when that exact name already exists.
         */
        auto transition_block_name(function_codegen_state const& state, llvm::BasicBlock* source_block, llvm::BasicBlock* target_block) -> std::string
        {
            std::string const base_name = source_block->getName().str() + ".transition." + target_block->getName().str();
            std::string block_name = base_name;
            std::size_t duplicate_index = 1;
            for (;;)
            {
                bool collision = false;
                for (llvm::BasicBlock const& existing_block : *state.function)
                {
                    if (existing_block.getName() == block_name)
                    {
                        collision = true;
                        break;
                    }
                }

                if (!collision)
                {
                    return block_name;
                }

                block_name = base_name + "." + std::to_string(duplicate_index++);
            }
        }

        /**
         * Creates an edge cleanup block only when the successor requires dropping live locals before control transfers.
         */
        auto cleanup_edge_target(
            function_codegen_state& state,
            llvm::BasicBlock* source_block,
            quxlang::vmir2::state_map const& current_state,
            quxlang::vmir2::state_map const& target_state,
            llvm::BasicBlock* target_block) -> llvm::BasicBlock*
        {
            if (!edge_needs_cleanup(state, current_state, target_state))
            {
                return target_block;
            }

            std::string const block_name = transition_block_name(state, source_block, target_block);
            llvm::BasicBlock* cleanup_block = llvm::BasicBlock::Create(context, block_name, state.function);
            llvm::IRBuilderBase::InsertPoint const saved_insert_point = builder.saveIP();
            builder.SetInsertPoint(cleanup_block);
            emit_transition_cleanup(state, builder, current_state, target_state);
            builder.CreateBr(target_block);
            builder.restoreIP(saved_insert_point);
            return cleanup_block;
        }

        void zero_initialize_slot(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot)
        {
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(slot)).type;
            llvm::Type* storage_type = value_storage_type(type);
            store_slot_value(state, ir_builder, slot, llvm::Constant::getNullValue(storage_type));
        }

        auto llvm_load_ordering(quxlang::atomic_access_mode mode) const -> std::optional< llvm::AtomicOrdering >
        {
            switch (mode)
            {
            case quxlang::atomic_access_mode::nonatomic:
                return std::nullopt;
            case quxlang::atomic_access_mode::atomic_relaxed:
                return llvm::AtomicOrdering::Monotonic;
            case quxlang::atomic_access_mode::atomic_acquire:
                return llvm::AtomicOrdering::Acquire;
            case quxlang::atomic_access_mode::atomic_seqcst:
                return llvm::AtomicOrdering::SequentiallyConsistent;
            case quxlang::atomic_access_mode::atomic_release:
            case quxlang::atomic_access_mode::atomic_acqrel:
                throw quxlang::semantic_compilation_error("Invalid atomic load ordering for LLVM lowering");
            }

            throw quxlang::semantic_compilation_error("Unknown atomic load ordering for LLVM lowering");
        }

        auto llvm_store_ordering(quxlang::atomic_access_mode mode) const -> std::optional< llvm::AtomicOrdering >
        {
            switch (mode)
            {
            case quxlang::atomic_access_mode::nonatomic:
                return std::nullopt;
            case quxlang::atomic_access_mode::atomic_relaxed:
                return llvm::AtomicOrdering::Monotonic;
            case quxlang::atomic_access_mode::atomic_release:
                return llvm::AtomicOrdering::Release;
            case quxlang::atomic_access_mode::atomic_seqcst:
                return llvm::AtomicOrdering::SequentiallyConsistent;
            case quxlang::atomic_access_mode::atomic_acquire:
            case quxlang::atomic_access_mode::atomic_acqrel:
                throw quxlang::semantic_compilation_error("Invalid atomic store ordering for LLVM lowering");
            }

            throw quxlang::semantic_compilation_error("Unknown atomic store ordering for LLVM lowering");
        }

        auto llvm_cmpxchg_success_ordering(quxlang::atomic_access_mode mode) const -> llvm::AtomicOrdering
        {
            switch (mode)
            {
            case quxlang::atomic_access_mode::atomic_relaxed:
                return llvm::AtomicOrdering::Monotonic;
            case quxlang::atomic_access_mode::atomic_release:
                return llvm::AtomicOrdering::Release;
            case quxlang::atomic_access_mode::atomic_acquire:
                return llvm::AtomicOrdering::Acquire;
            case quxlang::atomic_access_mode::atomic_acqrel:
                return llvm::AtomicOrdering::AcquireRelease;
            case quxlang::atomic_access_mode::atomic_seqcst:
                return llvm::AtomicOrdering::SequentiallyConsistent;
            case quxlang::atomic_access_mode::nonatomic:
                throw quxlang::semantic_compilation_error("Nonatomic compare_exchange cannot use atomic success ordering");
            }

            throw quxlang::semantic_compilation_error("Unknown compare_exchange success ordering for LLVM lowering");
        }

        auto llvm_cmpxchg_failure_ordering(quxlang::atomic_access_mode mode) const -> llvm::AtomicOrdering
        {
            switch (mode)
            {
            case quxlang::atomic_access_mode::atomic_relaxed:
                return llvm::AtomicOrdering::Monotonic;
            case quxlang::atomic_access_mode::atomic_acquire:
                return llvm::AtomicOrdering::Acquire;
            case quxlang::atomic_access_mode::atomic_seqcst:
                return llvm::AtomicOrdering::SequentiallyConsistent;
            case quxlang::atomic_access_mode::atomic_release:
            case quxlang::atomic_access_mode::atomic_acqrel:
            case quxlang::atomic_access_mode::nonatomic:
                throw quxlang::semantic_compilation_error("Invalid compare_exchange failure ordering for LLVM lowering");
            }

            throw quxlang::semantic_compilation_error("Unknown compare_exchange failure ordering for LLVM lowering");
        }

        /**
         * Converts a VMIR read-modify-write access mode into the corresponding LLVM atomic ordering.
         */
        auto llvm_rmw_ordering(quxlang::atomic_access_mode mode) const -> llvm::AtomicOrdering
        {
            switch (mode)
            {
            case quxlang::atomic_access_mode::atomic_relaxed:
                return llvm::AtomicOrdering::Monotonic;
            case quxlang::atomic_access_mode::atomic_release:
                return llvm::AtomicOrdering::Release;
            case quxlang::atomic_access_mode::atomic_acquire:
                return llvm::AtomicOrdering::Acquire;
            case quxlang::atomic_access_mode::atomic_acqrel:
                return llvm::AtomicOrdering::AcquireRelease;
            case quxlang::atomic_access_mode::atomic_seqcst:
                return llvm::AtomicOrdering::SequentiallyConsistent;
            case quxlang::atomic_access_mode::nonatomic:
                throw quxlang::semantic_compilation_error("Nonatomic read-modify-write cannot use atomic ordering");
            }

            throw quxlang::semantic_compilation_error("Unknown read-modify-write ordering for LLVM lowering");
        }

        /**
         * Chooses the failure ordering for the cmpxchg loop used to lower non-native-width atomic RMW operations.
         */
        auto llvm_rmw_cmpxchg_failure_ordering(quxlang::atomic_access_mode mode) const -> llvm::AtomicOrdering
        {
            switch (mode)
            {
            case quxlang::atomic_access_mode::atomic_relaxed:
            case quxlang::atomic_access_mode::atomic_release:
                return llvm::AtomicOrdering::Monotonic;
            case quxlang::atomic_access_mode::atomic_acquire:
            case quxlang::atomic_access_mode::atomic_acqrel:
                return llvm::AtomicOrdering::Acquire;
            case quxlang::atomic_access_mode::atomic_seqcst:
                return llvm::AtomicOrdering::SequentiallyConsistent;
            case quxlang::atomic_access_mode::nonatomic:
                throw quxlang::semantic_compilation_error("Nonatomic read-modify-write cannot use atomic cmpxchg failure ordering");
            }

            throw quxlang::semantic_compilation_error("Unknown read-modify-write cmpxchg failure ordering for LLVM lowering");
        }

        /**
         * Emits an LLVM atomicrmw instruction and optionally stores its returned old value.
         */
        void emit_atomic_rmw(
            function_codegen_state& state,
            llvm::BasicBlock*& current_block,
            quxlang::vmir2::local_index target,
            quxlang::vmir2::local_index value,
            quxlang::atomic_access_mode access_mode,
            std::optional< quxlang::vmir2::local_index > old_value,
            llvm::AtomicRMWInst::BinOp op)
        {
            llvm::Value* pointer = load_reference_pointer(state, builder, target);
            quxlang::type_symbol const atomic_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(target)).type);
            if (!quxlang::is_atomic_type(atomic_type))
            {
                throw quxlang::semantic_compilation_error("Atomic read-modify-write target is not an atomic type");
            }

            quxlang::type_symbol const logical_type = quxlang::atomic_storage_type_or_self(atomic_type);
            llvm::Type* const logical_llvm_type = value_storage_type(logical_type);
            llvm::Type* const storage_llvm_type = value_storage_type(atomic_type);
            std::uint64_t const storage_alignment = slot_alignment(atomic_type);
            if (storage_llvm_type->isIntegerTy() || storage_llvm_type->isPointerTy())
            {
                std::uint64_t const storage_bits = storage_llvm_type->isPointerTy()
                                                       ? input.machine_target.machine.pointer_size_bytes() * 8
                                                       : llvm::cast< llvm::IntegerType >(storage_llvm_type)->getBitWidth();
                if (storage_bits > input.machine_target.machine.max_native_atomic_storage_bits())
                {
                    throw quxlang::compiler_bug("Non-native atomic read-modify-write lowering is not implemented for storage width " + std::to_string(storage_bits));
                }
            }
            llvm::Value* rhs = integer_value(state, builder, value);
            if (logical_llvm_type == storage_llvm_type)
            {
                llvm::AtomicRMWInst* rmw = builder.CreateAtomicRMW(op, pointer, rhs, llvm::Align(storage_alignment), llvm_rmw_ordering(access_mode));
                rmw->setVolatile(false);
                if (old_value.has_value())
                {
                    store_slot_value(state, builder, *old_value, rmw);
                }
                return;
            }

            if (!logical_llvm_type->isIntegerTy() || !storage_llvm_type->isIntegerTy())
            {
                throw quxlang::semantic_compilation_error("Non-native-width atomic read-modify-write requires integer storage");
            }

            llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(context, "atomicrmw.loop", state.function);
            llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context, "atomicrmw.cont", state.function);
            builder.CreateBr(loop_block);

            builder.SetInsertPoint(loop_block);
            llvm::LoadInst* current_storage_load = builder.CreateLoad(storage_llvm_type, pointer);
            current_storage_load->setAtomic(llvm_rmw_cmpxchg_failure_ordering(access_mode));
            current_storage_load->setAlignment(llvm::Align(storage_alignment));
            llvm::Value* current_logical_value = storage_atomic_value_to_logical(builder, atomic_type, current_storage_load);
            llvm::Value* updated_logical_value = nullptr;
            switch (op)
            {
            case llvm::AtomicRMWInst::Add:
                updated_logical_value = builder.CreateAdd(current_logical_value, rhs);
                break;
            case llvm::AtomicRMWInst::Sub:
                updated_logical_value = builder.CreateSub(current_logical_value, rhs);
                break;
            case llvm::AtomicRMWInst::And:
                updated_logical_value = builder.CreateAnd(current_logical_value, rhs);
                break;
            case llvm::AtomicRMWInst::Or:
                updated_logical_value = builder.CreateOr(current_logical_value, rhs);
                break;
            case llvm::AtomicRMWInst::Xor:
                updated_logical_value = builder.CreateXor(current_logical_value, rhs);
                break;
            default:
                throw quxlang::semantic_compilation_error("Unsupported non-native-width atomic read-modify-write operation");
            }
            llvm::Value* updated_storage_value = logical_atomic_value_to_storage(builder, atomic_type, updated_logical_value);
            llvm::AtomicCmpXchgInst* cmpxchg = builder.CreateAtomicCmpXchg(
                pointer,
                current_storage_load,
                updated_storage_value,
                llvm::Align(storage_alignment),
                llvm_rmw_ordering(access_mode),
                llvm_rmw_cmpxchg_failure_ordering(access_mode));
            cmpxchg->setVolatile(false);
            llvm::Value* matched = builder.CreateExtractValue(cmpxchg, 1);
            builder.CreateCondBr(matched, continue_block, loop_block);

            current_block = continue_block;
            builder.SetInsertPoint(current_block);
            if (old_value.has_value())
            {
                store_slot_value(state, builder, *old_value, current_logical_value);
            }
        }

        auto parse_float_constant(llvm::Type* llvm_type, std::string const& text) -> llvm::Constant*
        {
            llvm::APFloat float_value(0.0);
            if (llvm_type->isHalfTy())
            {
                float_value = llvm::APFloat(llvm::APFloat::IEEEhalf(), text);
            }
            else if (llvm_type->isFloatTy())
            {
                float_value = llvm::APFloat(llvm::APFloat::IEEEsingle(), text);
            }
            else if (llvm_type->isDoubleTy())
            {
                float_value = llvm::APFloat(llvm::APFloat::IEEEdouble(), text);
            }
            else if (llvm_type->isX86_FP80Ty())
            {
                float_value = llvm::APFloat(llvm::APFloat::x87DoubleExtended(), text);
            }
            else if (llvm_type->isFP128Ty())
            {
                float_value = llvm::APFloat(llvm::APFloat::IEEEquad(), text);
            }
            else
            {
                throw quxlang::semantic_compilation_error("Unsupported LLVM float type in constant lowering");
            }

            return llvm::ConstantFP::get(context, float_value);
        }

        auto create_private_interface_constant(
            quxlang::type_symbol const& interface_type,
            std::map< quxlang::interface_slot_key, quxlang::type_symbol > const& functions_map,
            bool is_default_value) -> llvm::Constant*
        {
            llvm::StructType* struct_type = get_or_create_interface_struct(interface_type);
            std::vector< llvm::Constant* > fields;
            std::vector< quxlang::interface_slot_key > const& slots = get_interface_slots(interface_type);
            fields.reserve(slots.size() + 1);
            fields.push_back(llvm::ConstantInt::get(i8_type(), is_default_value ? 1 : 0));

            for (quxlang::interface_slot_key const& slot : slots)
            {
                std::map< quxlang::interface_slot_key, quxlang::type_symbol >::const_iterator function_iter = functions_map.find(slot);
                if (function_iter == functions_map.end())
                {
                    fields.push_back(llvm::ConstantPointerNull::get(opaque_pointer_type()));
                    continue;
                }

                callable_abi abi = interface_slot_abi(slot);
                llvm::Function* callee = get_or_create_external_function(function_iter->second, abi);
                fields.push_back(llvm::ConstantExpr::getPointerCast(callee, opaque_pointer_type()));
            }

            return llvm::ConstantStruct::get(struct_type, fields);
        }

        void assign_slot_alias(function_codegen_state& state, quxlang::vmir2::local_index slot, llvm::Value* address)
        {
            state.locals.at(local_slot_index(slot)).aliased_value_address = address;
        }

        /**
         * Lowers one variant-wrapped VMIR instruction by dispatching to a concrete overload.
         */
        void emit_instruction(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::vm_instruction const& instruction)
        {
            builder.SetInsertPoint(current_block);
            apply_debug_location(state, quxlang::vmir2::get_location(instruction));
            rpnx::apply_visitor< void >(
                instruction,
                [&](auto const& typed_instruction)
                {
                    emit_instruction_ovl(state, current_block, typed_instruction);
                });
        }

        /**
         * Lowers one concrete VMIR instruction alternative.
         */
        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::access_field const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::access_field const& inst = instruction;
            quxlang::type_symbol field_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.store_index)).type);
            store_reference_pointer(state, builder, inst.store_index, referenced_field_address(state, builder, inst.base_index, inst.field_name, field_type));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::interface_init const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::interface_init const& inst = instruction;
            llvm::Constant* value = create_private_interface_constant(inst.interface_type, inst.functions, inst.is_default);
            llvm::GlobalVariable* global = new llvm::GlobalVariable(
                *module,
                value->getType(),
                true,
                llvm::GlobalValue::PrivateLinkage,
                value,
                quxlang::to_string(inst.interface_type) + "$iface$" + std::to_string(helper_counter++));
            store_slot_value(state, builder, inst.target, builder.CreateBitCast(global, opaque_pointer_type()));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::interface_invoke const& instruction)
        {
            quxlang::vmir2::interface_invoke const& inst = instruction;
            quxlang::type_symbol interface_type = state.routine->local_types.at(local_slot_index(inst.interface_value)).type;
            llvm::StructType* struct_type = get_or_create_interface_struct(interface_type);
            std::size_t const field_index = interface_slot_indices.at(interface_type).at(inst.slot);

            llvm::Value* handle = load_slot_value(state, builder, inst.interface_value);
            llvm::Value* typed_handle = builder.CreateBitCast(handle, llvm::PointerType::get(context, 0));
            llvm::Value* is_default_ptr = builder.CreateStructGEP(struct_type, typed_handle, 0);
            llvm::Value* is_default = builder.CreateLoad(i8_type(), is_default_ptr);
            llvm::Value* fn_ptr_address = builder.CreateStructGEP(struct_type, typed_handle, field_index);
            llvm::Value* fn_ptr = builder.CreateLoad(opaque_pointer_type(), fn_ptr_address);

            llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context, "iface.cont", state.function);
            llvm::BasicBlock* dispatch_block = llvm::BasicBlock::Create(context, "iface.dispatch", state.function);
            llvm::BasicBlock* dispatch_call_block = llvm::BasicBlock::Create(context, "iface.call", state.function);
            llvm::BasicBlock* dispatch_trap_block = llvm::BasicBlock::Create(context, "iface.missing", state.function);
            llvm::BasicBlock* fallback_block = llvm::BasicBlock::Create(context, "iface.default", state.function);
            builder.CreateCondBr(builder.CreateICmpNE(is_default, llvm::ConstantInt::get(i8_type(), 0)), fallback_block, dispatch_block);

            builder.SetInsertPoint(dispatch_block);
            builder.CreateCondBr(builder.CreateICmpNE(fn_ptr, llvm::ConstantPointerNull::get(opaque_pointer_type())), dispatch_call_block, dispatch_trap_block);

            builder.SetInsertPoint(dispatch_call_block);
            callable_abi abi = interface_slot_abi(inst.slot);
            llvm::Value* typed_fn_ptr = builder.CreateBitCast(fn_ptr, llvm::PointerType::get(context, 0));
            if (abi.llvm_type->getReturnType()->isVoidTy())
            {
                apply_calling_convention(builder.CreateCall(abi.llvm_type, typed_fn_ptr, ordered_call_arguments(state, builder, abi, inst.args)), abi);
            }
            else
            {
                llvm::CallInst* call = builder.CreateCall(abi.llvm_type, typed_fn_ptr, ordered_call_arguments(state, builder, abi, inst.args));
                apply_calling_convention(call, abi);
                std::optional< quxlang::vmir2::local_index > return_slot = call_return_slot(abi, inst.args);
                if (!return_slot.has_value())
                {
                    throw quxlang::semantic_compilation_error("Missing VMIR return slot for interface invoke");
                }
                store_slot_value(state, builder, *return_slot, call);
            }
            builder.CreateBr(continue_block);

            builder.SetInsertPoint(dispatch_trap_block);
            llvm::Function* trap = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::trap);
            builder.CreateCall(trap);
            builder.CreateUnreachable();

            builder.SetInsertPoint(fallback_block);
            if (inst.default_function.has_value())
            {
                quxlang::vmir2::invocation_args const default_args = invocation_args_with_this(inst.args, inst.interface_value);
                callable_abi default_abi = direct_callee_abi(*inst.default_function, default_args, state);
                llvm::Function* fallback = get_or_create_external_function(*inst.default_function, default_abi);
                if (default_abi.llvm_type->getReturnType()->isVoidTy())
                {
                    apply_calling_convention(builder.CreateCall(default_abi.llvm_type, fallback, ordered_call_arguments(state, builder, default_abi, default_args)), default_abi);
                }
                else
                {
                    llvm::CallInst* call = builder.CreateCall(default_abi.llvm_type, fallback, ordered_call_arguments(state, builder, default_abi, default_args));
                    apply_calling_convention(call, default_abi);
                    std::optional< quxlang::vmir2::local_index > return_slot = call_return_slot(default_abi, default_args);
                    if (!return_slot.has_value())
                    {
                        throw quxlang::semantic_compilation_error("Missing VMIR return slot for interface default invoke");
                    }
                    store_slot_value(state, builder, *return_slot, call);
                }
                builder.CreateBr(continue_block);
            }
            else
            {
                llvm::Function* default_trap = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::trap);
                builder.CreateCall(default_trap);
                builder.CreateUnreachable();
            }

            current_block = continue_block;
            builder.SetInsertPoint(current_block);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::interface_is_default const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::interface_is_default const& inst = instruction;
            quxlang::type_symbol interface_type = state.routine->local_types.at(local_slot_index(inst.interface_value)).type;
            llvm::StructType* struct_type = get_or_create_interface_struct(interface_type);
            llvm::Value* handle = load_slot_value(state, builder, inst.interface_value);
            llvm::Value* typed_handle = builder.CreateBitCast(handle, llvm::PointerType::get(context, 0));
            llvm::Value* is_default_ptr = builder.CreateStructGEP(struct_type, typed_handle, 0);
            llvm::Value* is_default = builder.CreateLoad(i8_type(), is_default_ptr);
            store_boolean(state, builder, inst.result, builder.CreateICmpNE(is_default, llvm::ConstantInt::get(i8_type(), 0)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::invoke const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::invoke const& inst = instruction;
            callable_abi abi = direct_callee_abi(inst.what, inst, state);
            llvm::Function* callee = get_or_create_external_function(inst.what, abi);
            llvm::CallInst* call = builder.CreateCall(abi.llvm_type, callee, ordered_call_arguments(state, builder, abi, inst.args));
            apply_calling_convention(call, abi);
            if (std::optional< quxlang::vmir2::local_index > return_slot = call_return_slot(abi, inst.args); return_slot.has_value())
            {
                store_slot_value(state, builder, *return_slot, call);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::invoke_indirect const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::invoke_indirect const& inst = instruction;
            quxlang::type_symbol const original_slot_type = state.routine->local_types.at(local_slot_index(inst.what_index)).type;
            quxlang::type_symbol pointer_slot_type = original_slot_type;
            bool const is_reference_callable = quxlang::is_ref(pointer_slot_type);
            if (is_reference_callable)
            {
                pointer_slot_type = quxlang::remove_ref(pointer_slot_type);
            }
            bool const is_pointer_callable = quxlang::is_ptr(pointer_slot_type);
            bool const is_procedure_reference = is_reference_callable && pointer_slot_type.type_is< quxlang::procedure_type >();
            if (!is_pointer_callable && !is_procedure_reference)
            {
                throw quxlang::semantic_compilation_error("INVOKE_INDIRECT requires a procedure pointer or procedure reference");
            }
            quxlang::type_symbol callable_type = is_pointer_callable ? quxlang::remove_ptr(pointer_slot_type) : pointer_slot_type;
            if (!callable_type.type_is< quxlang::procedure_type >())
            {
                throw quxlang::semantic_compilation_error("INVOKE_INDIRECT requires a PROCEDURE pointer or reference");
            }

            callable_abi abi = callable_abi_from_signature(callable_type.get_as< quxlang::procedure_type >().signature);
            llvm::Value* callee_value = nullptr;
            if (is_reference_callable)
            {
                llvm::Value* pointer_to_pointer = load_reference_pointer(state, builder, inst.what_index);
                callee_value = builder.CreateLoad(opaque_pointer_type(), pointer_to_pointer);
            }
            else
            {
                callee_value = load_slot_value(state, builder, inst.what_index);
            }
            llvm::Value* typed_callee = builder.CreateBitCast(callee_value, llvm::PointerType::get(context, 0));
            llvm::CallInst* call = builder.CreateCall(abi.llvm_type, typed_callee, ordered_call_arguments(state, builder, abi, inst.args));
            apply_calling_convention(call, abi);
            if (std::optional< quxlang::vmir2::local_index > return_slot = call_return_slot(abi, inst.args); return_slot.has_value())
            {
                store_slot_value(state, builder, *return_slot, call);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::get_procedure_ptr const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::get_procedure_ptr const& inst = instruction;
            quxlang::type_symbol pointer_type = state.routine->local_types.at(local_slot_index(inst.pointer_index)).type;
            quxlang::type_symbol callable_type = quxlang::remove_ptr(pointer_type);
            if (!callable_type.type_is< quxlang::procedure_type >())
            {
                throw quxlang::semantic_compilation_error("GET_PROCEDURE_PTR target slot is not a procedure pointer");
            }

            callable_abi abi = callable_abi_from_signature(callable_type.get_as< quxlang::procedure_type >().signature);
            llvm::Function* function = get_or_create_external_function(inst.routine, abi);
            store_slot_value(state, builder, inst.pointer_index, builder.CreateBitCast(function, opaque_pointer_type()));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::make_reference const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::make_reference const& inst = instruction;
            store_reference_pointer(state, builder, inst.reference_index, value_address(state, inst.value_index));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::cast_ptrref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::cast_ptrref const& inst = instruction;
            store_reference_pointer(state, builder, inst.target_index, load_reference_pointer(state, builder, inst.source_index));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_set_result const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_SET_RESULT cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_set_result2 const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_SET_RESULT2 cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_make_proxy const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_MAKE_PROXY cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_output_byte const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_OUTPUT_BYTE cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_const_int const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::load_const_int const& inst = instruction;
            quxlang::type_symbol const& target_type = state.routine->local_types.at(local_slot_index(inst.target)).type;
            llvm::IntegerType* integer_type = llvm::cast< llvm::IntegerType >(value_storage_type(target_type));
            std::string digits = inst.value;
            bool is_negative = !digits.empty() && digits.front() == '-';
            if (is_negative)
            {
                digits.erase(digits.begin());
            }
            llvm::APInt value(integer_type->getBitWidth(), digits, 10);
            if (is_negative)
            {
                value = -value;
            }
            store_slot_value(state, builder, inst.target, llvm::ConstantInt::get(integer_type, value));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_const_float const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::load_const_float const& inst = instruction;
            quxlang::type_symbol const& target_type = state.routine->local_types.at(local_slot_index(inst.target)).type;
            llvm::Type* float_type = value_storage_type(target_type);
            store_slot_value(state, builder, inst.target, parse_float_constant(float_type, inst.value));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_const_value const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::load_const_value const& inst = instruction;
            store_readonly_constant_value(state, builder, inst.target, inst.value);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::canonicalize_float const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::canonicalize_float const& inst = instruction;
            quxlang::type_symbol const& source_type = state.routine->local_types.at(local_slot_index(inst.source)).type;
            quxlang::type_symbol const& result_type = state.routine->local_types.at(local_slot_index(inst.result)).type;
            if (source_type != result_type || !source_type.type_is< quxlang::float_type >())
            {
                throw quxlang::semantic_compilation_error("FCANON requires matching floating point source and result types");
            }

            quxlang::float_type const& float_info = source_type.as< quxlang::float_type >();
            unsigned const float_bits = float_info.bits;
            unsigned const exponent_bits = float_info.exponent_bits;
            unsigned const significand_bits = float_bits - exponent_bits - 1;

            llvm::Value* source_value = load_slot_value(state, builder, inst.source);
            llvm::IntegerType* integer_type = llvm::IntegerType::get(context, float_bits);
            llvm::Value* source_bits = builder.CreateBitCast(source_value, integer_type);

            llvm::APInt const exponent_mask = llvm::APInt::getBitsSet(float_bits, significand_bits, significand_bits + exponent_bits);
            llvm::APInt const significand_mask = llvm::APInt::getLowBitsSet(float_bits, significand_bits);
            llvm::APInt const canonical_nan_bits = llvm::APInt::getOneBitSet(float_bits, significand_bits == 0 ? 0 : significand_bits - 1) | exponent_mask;

            llvm::Value* exponent_bits_value = builder.CreateAnd(source_bits, llvm::ConstantInt::get(integer_type, exponent_mask));
            llvm::Value* significand_bits_value = builder.CreateAnd(source_bits, llvm::ConstantInt::get(integer_type, significand_mask));
            llvm::Value* has_all_exponent_bits = builder.CreateICmpEQ(exponent_bits_value, llvm::ConstantInt::get(integer_type, exponent_mask));
            llvm::Value* has_nonzero_significand = builder.CreateICmpNE(significand_bits_value, llvm::ConstantInt::get(integer_type, llvm::APInt(float_bits, 0)));
            llvm::Value* is_nan = builder.CreateAnd(has_all_exponent_bits, has_nonzero_significand);
            llvm::Value* canonicalized_bits = builder.CreateSelect(is_nan, llvm::ConstantInt::get(integer_type, canonical_nan_bits), source_bits);
            llvm::Value* canonicalized = builder.CreateBitCast(canonicalized_bits, source_value->getType());
            store_slot_value(state, builder, inst.result, canonicalized);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::get_value_byte const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::get_value_byte const& inst = instruction;
            llvm::Value* pointer = load_reference_pointer(state, builder, inst.source_reference);
            llvm::Value* byte_pointer = builder.CreateInBoundsGEP(i8_type(), builder.CreateBitCast(pointer, opaque_pointer_type()), llvm::ConstantInt::get(i64_type(), inst.offset));
            llvm::Value* byte_value = builder.CreateLoad(i8_type(), byte_pointer);
            store_slot_value(state, builder, inst.result, byte_value);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::set_value_byte const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::set_value_byte const& inst = instruction;
            llvm::Value* pointer = load_reference_pointer(state, builder, inst.target_reference);
            llvm::Value* byte_pointer = builder.CreateInBoundsGEP(i8_type(), builder.CreateBitCast(pointer, opaque_pointer_type()), llvm::ConstantInt::get(i64_type(), inst.offset));
            builder.CreateStore(load_slot_value(state, builder, inst.value), byte_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::make_pointer_to const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::make_pointer_to const& inst = instruction;
            quxlang::type_symbol const& source_type = state.routine->local_types.at(local_slot_index(inst.of_index)).type;
            llvm::Value* pointer_value = quxlang::is_ref(source_type)
                                             ? load_reference_pointer(state, builder, inst.of_index)
                                             : value_address(state, inst.of_index);
            store_slot_value(state, builder, inst.pointer_index, pointer_value);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_from_ref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::load_from_ref const& inst = instruction;
            quxlang::type_symbol reference_type = state.routine->local_types.at(local_slot_index(inst.from_reference)).type;
            quxlang::type_symbol value_type = quxlang::remove_ref(reference_type);
            llvm::Value* pointer_value = load_reference_pointer(state, builder, inst.from_reference);
            llvm::LoadInst* load = builder.CreateLoad(value_storage_type(value_type), pointer_value);
            if (std::optional< llvm::AtomicOrdering > const ordering = llvm_load_ordering(inst.access_mode); ordering.has_value())
            {
                llvm::Type* const storage_llvm_type = value_storage_type(value_type);
                if (storage_llvm_type->isIntegerTy() || storage_llvm_type->isPointerTy())
                {
                    std::uint64_t const storage_bits = storage_llvm_type->isPointerTy()
                                                           ? input.machine_target.machine.pointer_size_bytes() * 8
                                                           : llvm::cast< llvm::IntegerType >(storage_llvm_type)->getBitWidth();
                    if (storage_bits > input.machine_target.machine.max_native_atomic_storage_bits())
                    {
                        throw quxlang::compiler_bug("Non-native atomic load lowering is not implemented for storage width " + std::to_string(storage_bits));
                    }
                }
                load->setAtomic(*ordering);
                load->setAlignment(llvm::Align(slot_alignment(value_type)));
            }
            llvm::Value* loaded_value = load;
            if (quxlang::is_atomic_type(value_type))
            {
                loaded_value = storage_atomic_value_to_logical(builder, value_type, loaded_value);
            }
            store_slot_value(state, builder, inst.to_value, loaded_value);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::storage_init const& instruction)
        {
            (void)current_block;
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::storage_init_start const& instruction)
        {
            (void)current_block;
            llvm::Value* storage_pointer = load_reference_pointer(state, builder, instruction.on_storage);
            assign_slot_alias(state, instruction.target_value, storage_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::storage_deinit_start const& instruction)
        {
            (void)current_block;
            llvm::Value* storage_pointer = load_reference_pointer(state, builder, instruction.on_storage);
            assign_slot_alias(state, instruction.target_value, storage_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::storage_pun const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::storage_pun const& inst = instruction;
            store_reference_pointer(state, builder, inst.to_reference, load_reference_pointer(state, builder, inst.from_storage));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_alloc const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_ALLOC cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_alloc_multiple const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_ALLOC_MULTIPLE cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_dealloc const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_DEALLOC cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::constexpr_dealloc_multiple const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("CONSTEXPR_DEALLOC_MULTIPLE cannot be lowered to native code");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::get_object_ref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::get_object_ref const& inst = instruction;
            quxlang::type_symbol target_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.target_ref)).type);
            llvm::GlobalVariable* global = nullptr;
            switch (inst.type)
            {
            case quxlang::vmir2::access_type::storage:
            case quxlang::vmir2::access_type::object:
                global = get_or_create_common_zero_initialized_global(inst.symbol, value_storage_type(target_type));
                apply_access_class(global, inst.class_);
                store_reference_pointer(state, builder, inst.target_ref, global);
                return;
            }
            throw quxlang::compiler_bug("unknown GET_OBJECT_REF access type");
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::get_antestatal_ref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::get_antestatal_ref const& inst = instruction;
            quxlang::type_symbol target_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.target_ref)).type);
            llvm::GlobalVariable* global = get_or_create_constant_global(inst.symbol, target_type);
            store_reference_pointer(state, builder, inst.target_ref, global);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::initguard_global_get_ref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::initguard_global_get_ref const& inst = instruction;
            store_reference_pointer(state, builder, inst.target_ref, get_or_create_initguard_global(inst.symbol, quxlang::vmir2::access_class::global));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::initguard_complete const& instruction)
        {
            (void)current_block;
            emit_initguard_runtime_call(state, builder, instruction.lock, false);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::initguard_abort const& instruction)
        {
            (void)current_block;
            emit_initguard_runtime_call(state, builder, instruction.lock, true);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_const_zero const& instruction)
        {
            (void)current_block;
            zero_initialize_slot(state, builder, instruction.target);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_const_bool const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::load_const_bool const& inst = instruction;
            store_slot_value(state, builder, inst.target, llvm::ConstantInt::get(bool_storage_type(), inst.value ? 1 : 0));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::dereference_pointer const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::dereference_pointer const& inst = instruction;
            store_reference_pointer(state, builder, inst.to_reference, load_slot_value(state, builder, inst.from_pointer));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::store_to_ref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::store_to_ref const& inst = instruction;
            llvm::Value* destination = load_reference_pointer(state, builder, inst.to_reference);
            quxlang::type_symbol const destination_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.to_reference)).type);
            llvm::Value* source_value = load_slot_value(state, builder, inst.from_value);
            if (quxlang::is_atomic_type(destination_type))
            {
                source_value = logical_atomic_value_to_storage(builder, destination_type, source_value);
            }
            llvm::StoreInst* store = builder.CreateStore(source_value, destination);
            if (std::optional< llvm::AtomicOrdering > const ordering = llvm_store_ordering(inst.access_mode); ordering.has_value())
            {
                llvm::Type* const storage_llvm_type = value_storage_type(destination_type);
                if (storage_llvm_type->isIntegerTy() || storage_llvm_type->isPointerTy())
                {
                    std::uint64_t const storage_bits = storage_llvm_type->isPointerTy()
                                                           ? input.machine_target.machine.pointer_size_bytes() * 8
                                                           : llvm::cast< llvm::IntegerType >(storage_llvm_type)->getBitWidth();
                    if (storage_bits > input.machine_target.machine.max_native_atomic_storage_bits())
                    {
                        throw quxlang::compiler_bug("Non-native atomic store lowering is not implemented for storage width " + std::to_string(storage_bits));
                    }
                }
                store->setAtomic(*ordering);
                store->setAlignment(llvm::Align(slot_alignment(destination_type)));
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::compare_exchange const& instruction)
        {
            quxlang::vmir2::compare_exchange const& inst = instruction;
            quxlang::type_symbol const atomic_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.target_reference)).type);
            quxlang::type_symbol const target_type = quxlang::atomic_storage_type_or_self(atomic_type);
            llvm::Value* target_pointer = load_reference_pointer(state, builder, inst.target_reference);
            llvm::Value* expected_pointer = load_reference_pointer(state, builder, inst.expected_reference);
            llvm::Value* desired_value = load_slot_value(state, builder, inst.desired_value);
            llvm::Type* storage_type = value_storage_type(atomic_type);

            bool const use_atomic_cmpxchg = inst.success_mode != quxlang::atomic_access_mode::nonatomic || inst.failure_mode != quxlang::atomic_access_mode::nonatomic;
            if (use_atomic_cmpxchg)
            {
                if (!(storage_type->isIntegerTy() || storage_type->isPointerTy()))
                {
                    throw quxlang::semantic_compilation_error("Atomic compare_exchange requires integer or pointer storage in LLVM lowering");
                }
                std::uint64_t const storage_bits = storage_type->isPointerTy()
                                                       ? input.machine_target.machine.pointer_size_bytes() * 8
                                                       : llvm::cast< llvm::IntegerType >(storage_type)->getBitWidth();
                if (storage_bits > input.machine_target.machine.max_native_atomic_storage_bits())
                {
                    throw quxlang::compiler_bug("Non-native atomic compare_exchange lowering is not implemented for storage width " + std::to_string(storage_bits));
                }

                llvm::Value* expected_value = builder.CreateLoad(value_storage_type(target_type), expected_pointer);
                expected_value = logical_atomic_value_to_storage(builder, atomic_type, expected_value);
                desired_value = logical_atomic_value_to_storage(builder, atomic_type, desired_value);
                llvm::AtomicCmpXchgInst* cmpxchg = builder.CreateAtomicCmpXchg(
                    target_pointer,
                    expected_value,
                    desired_value,
                    llvm::Align(slot_alignment(atomic_type)),
                    llvm_cmpxchg_success_ordering(inst.success_mode),
                    llvm_cmpxchg_failure_ordering(inst.failure_mode));
                cmpxchg->setVolatile(false);

                llvm::Value* observed_value = builder.CreateExtractValue(cmpxchg, 0);
                llvm::Value* matched = builder.CreateExtractValue(cmpxchg, 1);

                llvm::BasicBlock* failure_block = llvm::BasicBlock::Create(context, "cmpxchg.failure", state.function);
                llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context, "cmpxchg.cont", state.function);
                builder.CreateCondBr(matched, continue_block, failure_block);

                builder.SetInsertPoint(failure_block);
                builder.CreateStore(storage_atomic_value_to_logical(builder, atomic_type, observed_value), expected_pointer);
                builder.CreateBr(continue_block);

                current_block = continue_block;
                builder.SetInsertPoint(current_block);
                store_boolean(state, builder, inst.result, matched);
                return;
            }

            llvm::Value* observed_value = builder.CreateLoad(storage_type, target_pointer);
            llvm::Value* expected_value = builder.CreateLoad(value_storage_type(target_type), expected_pointer);
            if (quxlang::is_atomic_type(atomic_type))
            {
                expected_value = logical_atomic_value_to_storage(builder, atomic_type, expected_value);
                desired_value = logical_atomic_value_to_storage(builder, atomic_type, desired_value);
            }
            llvm::Value* matched = nullptr;
            if (storage_type->isIntegerTy() || storage_type->isPointerTy())
            {
                matched = builder.CreateICmpEQ(observed_value, expected_value);
            }
            else if (storage_type->isFloatingPointTy())
            {
                matched = builder.CreateFCmpOEQ(observed_value, expected_value);
            }
            else
            {
                throw quxlang::semantic_compilation_error("COMPARE_EXCHANGE currently requires scalar or pointer storage in LLVM lowering");
            }

            llvm::BasicBlock* success_block = llvm::BasicBlock::Create(context, "cmpxchg.success", state.function);
            llvm::BasicBlock* failure_block = llvm::BasicBlock::Create(context, "cmpxchg.failure", state.function);
            llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context, "cmpxchg.cont", state.function);
            builder.CreateCondBr(matched, success_block, failure_block);

            builder.SetInsertPoint(success_block);
            builder.CreateStore(desired_value, target_pointer);
            builder.CreateBr(continue_block);

            builder.SetInsertPoint(failure_block);
            if (quxlang::is_atomic_type(atomic_type))
            {
                observed_value = storage_atomic_value_to_logical(builder, atomic_type, observed_value);
            }
            builder.CreateStore(observed_value, expected_pointer);
            builder.CreateBr(continue_block);

            current_block = continue_block;
            builder.SetInsertPoint(current_block);
            store_boolean(state, builder, inst.result, matched);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_add const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateAdd(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_mul const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateMul(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_div const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            bool is_signed = true;
            if (type.type_is< quxlang::int_type >())
            {
                is_signed = type.get_as< quxlang::int_type >().has_sign;
            }
            else if (type.type_is< quxlang::size_type >())
            {
                is_signed = false;
            }
            store_slot_value(state, builder, instruction.result, is_signed ? builder.CreateSDiv(lhs, rhs) : builder.CreateUDiv(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_mod const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            bool is_signed = true;
            if (type.type_is< quxlang::int_type >())
            {
                is_signed = type.get_as< quxlang::int_type >().has_sign;
            }
            else if (type.type_is< quxlang::size_type >())
            {
                is_signed = false;
            }
            store_slot_value(state, builder, instruction.result, is_signed ? builder.CreateSRem(lhs, rhs) : builder.CreateURem(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_sub const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateSub(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_int_add const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                emit_atomic_rmw(state, current_block, instruction.target, instruction.value, instruction.access_mode, instruction.old_value, llvm::AtomicRMWInst::Add);
                return;
            }
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = integer_value(state, builder, instruction.value);
            builder.CreateStore(builder.CreateAdd(current_value, rhs), pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_int_sub const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                emit_atomic_rmw(state, current_block, instruction.target, instruction.value, instruction.access_mode, instruction.old_value, llvm::AtomicRMWInst::Sub);
                return;
            }
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = integer_value(state, builder, instruction.value);
            builder.CreateStore(builder.CreateSub(current_value, rhs), pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_int_mul const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic integer multiplication is not supported by LLVM lowering");
            }
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = integer_value(state, builder, instruction.value);
            builder.CreateStore(builder.CreateMul(current_value, rhs), pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_int_div const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic integer division is not supported by LLVM lowering");
            }
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = integer_value(state, builder, instruction.value);
            bool is_signed = pointee_type.type_is< quxlang::int_type >() && pointee_type.get_as< quxlang::int_type >().has_sign;
            builder.CreateStore(is_signed ? builder.CreateSDiv(current_value, rhs) : builder.CreateUDiv(current_value, rhs), pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_int_mod const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic integer modulo is not supported by LLVM lowering");
            }
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = integer_value(state, builder, instruction.value);
            bool is_signed = pointee_type.type_is< quxlang::int_type >() && pointee_type.get_as< quxlang::int_type >().has_sign;
            builder.CreateStore(is_signed ? builder.CreateSRem(current_value, rhs) : builder.CreateURem(current_value, rhs), pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_add const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateFAdd(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_sub const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateFSub(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_mul const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateFMul(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_div const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateFDiv(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_float_add const& instruction)
        {
            (void)current_block;
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.value);
            builder.CreateStore(builder.CreateFAdd(current_value, rhs), pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_float_sub const& instruction)
        {
            (void)current_block;
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.value);
            builder.CreateStore(builder.CreateFSub(current_value, rhs), pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_float_mul const& instruction)
        {
            (void)current_block;
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.value);
            builder.CreateStore(builder.CreateFMul(current_value, rhs), pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_float_div const& instruction)
        {
            (void)current_block;
            llvm::Value* pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), pointer);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.value);
            builder.CreateStore(builder.CreateFDiv(current_value, rhs), pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_from_int const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::float_from_int const& inst = instruction;
            quxlang::type_symbol const& source_type = state.routine->local_types.at(local_slot_index(inst.source)).type;
            quxlang::type_symbol const& target_type = state.routine->local_types.at(local_slot_index(inst.result)).type;
            llvm::Value* integer = integer_value(state, builder, inst.source);
            llvm::Type* llvm_float_type = value_storage_type(target_type);
            llvm::Value* converted = nullptr;
            if (source_type.type_is< quxlang::int_type >() && source_type.get_as< quxlang::int_type >().has_sign)
            {
                converted = builder.CreateSIToFP(integer, llvm_float_type);
            }
            else
            {
                converted = builder.CreateUIToFP(integer, llvm_float_type);
            }
            store_slot_value(state, builder, inst.result, converted);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::iconv const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::iconv const& inst = instruction;
            quxlang::type_symbol const& source_type = state.routine->local_types.at(local_slot_index(inst.from)).type;
            quxlang::type_symbol const& target_type = state.routine->local_types.at(local_slot_index(inst.to)).type;
            llvm::Value* value = integer_value(state, builder, inst.from);
            llvm::IntegerType* destination_type = llvm::cast< llvm::IntegerType >(value_storage_type(target_type));
            llvm::Value* converted = value;
            if (value->getType() == destination_type)
            {
                converted = value;
            }
            else if (llvm::cast< llvm::IntegerType >(value->getType())->getBitWidth() > destination_type->getBitWidth())
            {
                converted = builder.CreateTrunc(value, destination_type);
            }
            else
            {
                bool signed_source = source_type.type_is< quxlang::int_type >() && source_type.get_as< quxlang::int_type >().has_sign;
                converted = signed_source ? builder.CreateSExt(value, destination_type) : builder.CreateZExt(value, destination_type);
            }
            store_slot_value(state, builder, inst.to, converted);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_and const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateAnd(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_or const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateOr(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_xor const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateXor(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_nand const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateNot(builder.CreateAnd(lhs, rhs)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_nor const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateNot(builder.CreateOr(lhs, rhs)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_nxor const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateNot(builder.CreateXor(lhs, rhs)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_implies const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateOr(builder.CreateNot(lhs), rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_implied const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            if (lhs->getType() != rhs->getType())
            {
                throw quxlang::semantic_compilation_error("Bitwise operands have mismatched LLVM types for lowering");
            }
            store_slot_value(state, builder, instruction.result, builder.CreateOr(lhs, builder.CreateNot(rhs)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_shift_up const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.value);
            llvm::Value* rhs = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(lhs->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs->getType());
            if (lhs_type != rhs_type)
            {
                rhs = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs, lhs_type) : builder.CreateZExt(rhs, lhs_type);
            }
            store_slot_value(state, builder, instruction.result, builder.CreateShl(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_shift_down const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.value);
            llvm::Value* rhs = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(lhs->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs->getType());
            if (lhs_type != rhs_type)
            {
                rhs = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs, lhs_type) : builder.CreateZExt(rhs, lhs_type);
            }
            store_slot_value(state, builder, instruction.result, builder.CreateLShr(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_rotate_up const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.value);
            llvm::Value* rhs = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(lhs->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs->getType());
            if (lhs_type != rhs_type)
            {
                rhs = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs, lhs_type) : builder.CreateZExt(rhs, lhs_type);
            }
            llvm::Function* rotl = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::fshl, {lhs->getType()});
            store_slot_value(state, builder, instruction.result, builder.CreateCall(rotl, {lhs, lhs, rhs}));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_rotate_down const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = integer_value(state, builder, instruction.value);
            llvm::Value* rhs = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(lhs->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs->getType());
            if (lhs_type != rhs_type)
            {
                rhs = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs, lhs_type) : builder.CreateZExt(rhs, lhs_type);
            }
            llvm::Function* rotr = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::fshr, {lhs->getType()});
            store_slot_value(state, builder, instruction.result, builder.CreateCall(rotr, {lhs, lhs, rhs}));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::bitwise_inverse const& instruction)
        {
            (void)current_block;
                quxlang::vmir2::bitwise_inverse const& inst = instruction;
                store_slot_value(state, builder, inst.result, builder.CreateNot(integer_value(state, builder, inst.value)));
                return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_and const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                emit_atomic_rmw(state, current_block, instruction.target, instruction.value, instruction.access_mode, instruction.old_value, llvm::AtomicRMWInst::And);
                return;
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateAnd(current_value, rhs_value), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_or const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                emit_atomic_rmw(state, current_block, instruction.target, instruction.value, instruction.access_mode, instruction.old_value, llvm::AtomicRMWInst::Or);
                return;
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateOr(current_value, rhs_value), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_xor const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                emit_atomic_rmw(state, current_block, instruction.target, instruction.value, instruction.access_mode, instruction.old_value, llvm::AtomicRMWInst::Xor);
                return;
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateXor(current_value, rhs_value), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_nand const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic bitwise NAND is not supported by LLVM lowering");
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateNot(builder.CreateAnd(current_value, rhs_value)), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_nor const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic bitwise NOR is not supported by LLVM lowering");
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateNot(builder.CreateOr(current_value, rhs_value)), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_nxor const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic bitwise NXOR is not supported by LLVM lowering");
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateNot(builder.CreateXor(current_value, rhs_value)), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_implies const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic bitwise implies is not supported by LLVM lowering");
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateOr(builder.CreateNot(current_value), rhs_value), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_implied const& instruction)
        {
            (void)current_block;
            if (instruction.access_mode != quxlang::atomic_access_mode::nonatomic)
            {
                throw quxlang::semantic_compilation_error("Atomic bitwise implied is not supported by LLVM lowering");
            }
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::atomic_storage_type_or_self(quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type));
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.value);
            if (current_value->getType() != rhs_value->getType())
            {
                throw quxlang::semantic_compilation_error("Mutating bitwise operands have mismatched LLVM types for lowering");
            }
            builder.CreateStore(builder.CreateOr(current_value, builder.CreateNot(rhs_value)), target_pointer);
            if (instruction.old_value.has_value())
            {
                store_slot_value(state, builder, *instruction.old_value, current_value);
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_shift_up const& instruction)
        {
            (void)current_block;
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(current_value->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs_value->getType());
            if (lhs_type != rhs_type)
            {
                rhs_value = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs_value, lhs_type) : builder.CreateZExt(rhs_value, lhs_type);
            }
            builder.CreateStore(builder.CreateShl(current_value, rhs_value), target_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_shift_down const& instruction)
        {
            (void)current_block;
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(current_value->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs_value->getType());
            if (lhs_type != rhs_type)
            {
                rhs_value = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs_value, lhs_type) : builder.CreateZExt(rhs_value, lhs_type);
            }
            builder.CreateStore(builder.CreateLShr(current_value, rhs_value), target_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_rotate_up const& instruction)
        {
            (void)current_block;
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(current_value->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs_value->getType());
            if (lhs_type != rhs_type)
            {
                rhs_value = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs_value, lhs_type) : builder.CreateZExt(rhs_value, lhs_type);
            }
            llvm::Function* rotl = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::fshl, {current_value->getType()});
            builder.CreateStore(builder.CreateCall(rotl, {current_value, current_value, rhs_value}), target_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::mut_bitwise_rotate_down const& instruction)
        {
            (void)current_block;
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            quxlang::type_symbol pointee_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(instruction.target)).type);
            llvm::Value* current_value = builder.CreateLoad(value_storage_type(pointee_type), target_pointer);
            llvm::Value* rhs_value = integer_value(state, builder, instruction.amount);
            llvm::IntegerType* lhs_type = llvm::cast< llvm::IntegerType >(current_value->getType());
            llvm::IntegerType* rhs_type = llvm::cast< llvm::IntegerType >(rhs_value->getType());
            if (lhs_type != rhs_type)
            {
                rhs_value = rhs_type->getBitWidth() > lhs_type->getBitWidth() ? builder.CreateTrunc(rhs_value, lhs_type) : builder.CreateZExt(rhs_value, lhs_type);
            }
            llvm::Function* rotr = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::fshr, {current_value->getType()});
            builder.CreateStore(builder.CreateCall(rotr, {current_value, current_value, rhs_value}), target_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::cmp_lt const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            llvm::Value* cmp = nullptr;
            if (type.type_is< quxlang::float_type >())
            {
                quxlang::float_type const& float_info = type.get_as< quxlang::float_type >();
                llvm::Value* lhs_key = float_total_order_key(lhs, float_info, builder);
                llvm::Value* rhs_key = float_total_order_key(rhs, float_info, builder);
                store_boolean(state, builder, instruction.result, builder.CreateICmpULT(lhs_key, rhs_key));
                return;
            }
            bool is_signed = true;
            if (type.type_is< quxlang::int_type >())
            {
                is_signed = type.get_as< quxlang::int_type >().has_sign;
            }
            else if (type.type_is< quxlang::size_type >() || nominal_integer_runtime_type(type))
            {
                is_signed = false;
            }
            cmp = is_signed ? builder.CreateICmpSLT(lhs, rhs) : builder.CreateICmpULT(lhs, rhs);
            store_boolean(state, builder, instruction.result, cmp);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::cmp_ge const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            if (type.type_is< quxlang::float_type >())
            {
                quxlang::float_type const& float_info = type.get_as< quxlang::float_type >();
                llvm::Value* lhs_key = float_total_order_key(lhs, float_info, builder);
                llvm::Value* rhs_key = float_total_order_key(rhs, float_info, builder);
                store_boolean(state, builder, instruction.result, builder.CreateICmpUGE(lhs_key, rhs_key));
                return;
            }
            bool is_signed = true;
            if (type.type_is< quxlang::int_type >())
            {
                is_signed = type.get_as< quxlang::int_type >().has_sign;
            }
            else if (type.type_is< quxlang::size_type >() || nominal_integer_runtime_type(type))
            {
                is_signed = false;
            }
            store_boolean(state, builder, instruction.result, is_signed ? builder.CreateICmpSGE(lhs, rhs) : builder.CreateICmpUGE(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::cmp_eq const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            if (type.type_is< quxlang::float_type >())
            {
                quxlang::float_type const& float_info = type.get_as< quxlang::float_type >();
                llvm::Value* lhs_key = float_total_order_key(lhs, float_info, builder);
                llvm::Value* rhs_key = float_total_order_key(rhs, float_info, builder);
                store_boolean(state, builder, instruction.result, builder.CreateICmpEQ(lhs_key, rhs_key));
                return;
            }
            store_boolean(state, builder, instruction.result, builder.CreateICmpEQ(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::cmp_ne const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            if (type.type_is< quxlang::float_type >())
            {
                quxlang::float_type const& float_info = type.get_as< quxlang::float_type >();
                llvm::Value* lhs_key = float_total_order_key(lhs, float_info, builder);
                llvm::Value* rhs_key = float_total_order_key(rhs, float_info, builder);
                store_boolean(state, builder, instruction.result, builder.CreateICmpNE(lhs_key, rhs_key));
                return;
            }
            store_boolean(state, builder, instruction.result, builder.CreateICmpNE(lhs, rhs));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_ieee_eq const& instruction)
        {
            (void)current_block;
            store_boolean(state, builder, instruction.result, builder.CreateFCmpOEQ(load_slot_value(state, builder, instruction.a), load_slot_value(state, builder, instruction.b)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_ieee_ne const& instruction)
        {
            (void)current_block;
            store_boolean(state, builder, instruction.result, builder.CreateFCmpUNE(load_slot_value(state, builder, instruction.a), load_slot_value(state, builder, instruction.b)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_ieee_lt const& instruction)
        {
            (void)current_block;
            store_boolean(state, builder, instruction.result, builder.CreateFCmpOLT(load_slot_value(state, builder, instruction.a), load_slot_value(state, builder, instruction.b)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_ieee_gt const& instruction)
        {
            (void)current_block;
            store_boolean(state, builder, instruction.result, builder.CreateFCmpOGT(load_slot_value(state, builder, instruction.a), load_slot_value(state, builder, instruction.b)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pcmp_lt const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpULT(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pcmp_ge const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpUGE(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pcmp_eq const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpEQ(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pcmp_ne const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpNE(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::gcmp_lt const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpULT(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::gcmp_ge const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpUGE(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::gcmp_eq const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpEQ(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::gcmp_ne const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.a), pointer_integer_type());
            llvm::Value* rhs_address = builder.CreatePtrToInt(load_slot_value(state, builder, instruction.b), pointer_integer_type());
            store_boolean(state, builder, instruction.result, builder.CreateICmpNE(lhs_address, rhs_address));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::defer_nontrivial_dtor const& instruction)
        {
            (void)current_block;
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::struct_init_start const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::struct_init_start const& inst = instruction;
            llvm::Value* base_pointer = value_address(state, inst.on_value);
            quxlang::type_symbol base_type = state.routine->local_types.at(local_slot_index(inst.on_value)).type;
            std::map< quxlang::type_symbol, quxlang::class_layout >::const_iterator layout_iter = input.class_layouts.find(base_type);
            if (layout_iter == input.class_layouts.end())
            {
                throw quxlang::semantic_compilation_error("Missing class layout for STRUCT_INIT_START on " + quxlang::to_string(base_type));
            }

            for (std::pair< std::string const, quxlang::vmir2::local_index > const& field_binding : inst.fields.named)
            {
                for (quxlang::class_field_info const& field : layout_iter->second.fields)
                {
                    if (field.name == field_binding.first)
                    {
                        llvm::Value* byte_pointer = builder.CreateBitCast(base_pointer, opaque_pointer_type());
                        llvm::Value* field_pointer = builder.CreateInBoundsGEP(i8_type(), byte_pointer, llvm::ConstantInt::get(i64_type(), field.offset));
                        assign_slot_alias(state, field_binding.second, field_pointer);
                        break;
                    }
                }
            }
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::struct_init_finish const& instruction)
        {
            (void)current_block;
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::copy_reference const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::copy_reference const& inst = instruction;
            store_reference_pointer(state, builder, inst.to_index, load_reference_pointer(state, builder, inst.from_index));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::destroy const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::local_index const slot = instruction.of;
            emit_slot_destructor_call(state, builder, slot);
            poison_slot_storage(state, builder, slot);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::end_lifetime const& instruction)
        {
            (void)current_block;
            poison_slot_storage(state, builder, instruction.of);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::access_array const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::access_array const& inst = instruction;
            quxlang::type_symbol base_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.base_index)).type);
            if (!base_type.type_is< quxlang::array_type >())
            {
                throw quxlang::semantic_compilation_error("ACCESS_ARRAY requires an array reference");
            }

            quxlang::array_type const& array_type = base_type.get_as< quxlang::array_type >();
            llvm::Value* base_pointer = load_reference_pointer(state, builder, inst.base_index);
            llvm::Value* index_value = integer_value(state, builder, inst.index_index);
            llvm::Value* byte_pointer = builder.CreateBitCast(base_pointer, opaque_pointer_type());
            std::uint64_t const element_size = slot_size(array_type.element_type);
            llvm::Value* byte_offset = builder.CreateMul(builder.CreateZExtOrTrunc(index_value, i64_type()), llvm::ConstantInt::get(i64_type(), element_size));
            llvm::Value* element_pointer = builder.CreateInBoundsGEP(i8_type(), byte_pointer, byte_offset);
            store_reference_pointer(state, builder, inst.store_index, element_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::access_pointer const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::access_pointer const& inst = instruction;
            quxlang::type_symbol pointer_type = state.routine->local_types.at(local_slot_index(inst.base_index)).type;
            quxlang::type_symbol element_type = quxlang::remove_ptr(pointer_type);
            llvm::Value* base_pointer = load_slot_value(state, builder, inst.base_index);
            llvm::Value* index_value = builder.CreateZExtOrTrunc(integer_value(state, builder, inst.index_index), i64_type());
            llvm::Value* byte_pointer = builder.CreateBitCast(base_pointer, opaque_pointer_type());
            llvm::Value* byte_offset = builder.CreateMul(index_value, llvm::ConstantInt::get(i64_type(), slot_size(element_type)));
            llvm::Value* element_pointer = builder.CreateInBoundsGEP(i8_type(), byte_pointer, byte_offset);
            store_reference_pointer(state, builder, inst.store_index, element_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::to_bool const& instruction)
        {
            (void)current_block;
            store_boolean(state, builder, instruction.to, truth_value(state, builder, instruction.from));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::to_bool_not const& instruction)
        {
            (void)current_block;
            store_boolean(state, builder, instruction.to, builder.CreateNot(truth_value(state, builder, instruction.from)));
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::increment const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& reference_type = state.routine->local_types.at(local_slot_index(instruction.value)).type;
            if (!quxlang::is_ref(reference_type))
            {
                throw quxlang::semantic_compilation_error("INC/DEC requires a reference slot, got " + quxlang::to_string(reference_type));
            }
            quxlang::type_symbol pointee_type = quxlang::remove_ref(reference_type);
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.value);
            if (pointee_type.type_is< quxlang::int_type >() || pointee_type.type_is< quxlang::bool_type >() || pointee_type.type_is< quxlang::byte_type >() || pointee_type.type_is< quxlang::size_type >())
            {
                llvm::Type* llvm_value_type = value_storage_type(pointee_type);
                llvm::Value* old_value = builder.CreateLoad(llvm_value_type, target_pointer);
                llvm::Value* updated_value = builder.CreateAdd(old_value, scalar_one(llvm_value_type));
                builder.CreateStore(updated_value, target_pointer);
                store_slot_value(state, builder, instruction.result, old_value);
                return;
            }
            if (quxlang::is_ptr(pointee_type))
            {
                quxlang::type_symbol element_type = quxlang::remove_ptr(pointee_type);
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::getSigned(i64_type(), static_cast< std::int64_t >(slot_size(element_type))));
                builder.CreateStore(updated_pointer, target_pointer);
                store_slot_value(state, builder, instruction.result, old_pointer);
                return;
            }
            throw quxlang::semantic_compilation_error("INC/DEC requires a reference to an integer or pointer, got " + quxlang::to_string(reference_type));
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::decrement const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& reference_type = state.routine->local_types.at(local_slot_index(instruction.value)).type;
            if (!quxlang::is_ref(reference_type))
            {
                throw quxlang::semantic_compilation_error("INC/DEC requires a reference slot, got " + quxlang::to_string(reference_type));
            }
            quxlang::type_symbol pointee_type = quxlang::remove_ref(reference_type);
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.value);
            if (pointee_type.type_is< quxlang::int_type >() || pointee_type.type_is< quxlang::bool_type >() || pointee_type.type_is< quxlang::byte_type >() || pointee_type.type_is< quxlang::size_type >())
            {
                llvm::Type* llvm_value_type = value_storage_type(pointee_type);
                llvm::Value* old_value = builder.CreateLoad(llvm_value_type, target_pointer);
                llvm::Value* updated_value = builder.CreateSub(old_value, scalar_one(llvm_value_type));
                builder.CreateStore(updated_value, target_pointer);
                store_slot_value(state, builder, instruction.result, old_value);
                return;
            }
            if (quxlang::is_ptr(pointee_type))
            {
                quxlang::type_symbol element_type = quxlang::remove_ptr(pointee_type);
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::getSigned(i64_type(), -static_cast< std::int64_t >(slot_size(element_type))));
                builder.CreateStore(updated_pointer, target_pointer);
                store_slot_value(state, builder, instruction.result, old_pointer);
                return;
            }
            throw quxlang::semantic_compilation_error("INC/DEC requires a reference to an integer or pointer, got " + quxlang::to_string(reference_type));
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::preincrement const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& reference_type = state.routine->local_types.at(local_slot_index(instruction.target)).type;
            if (!quxlang::is_ref(reference_type))
            {
                throw quxlang::semantic_compilation_error("PREINC/PREDEC requires a reference slot, got " + quxlang::to_string(reference_type));
            }
            quxlang::type_symbol pointee_type = quxlang::remove_ref(reference_type);
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            if (pointee_type.type_is< quxlang::int_type >() || pointee_type.type_is< quxlang::bool_type >() || pointee_type.type_is< quxlang::byte_type >() || pointee_type.type_is< quxlang::size_type >())
            {
                llvm::Type* llvm_value_type = value_storage_type(pointee_type);
                llvm::Value* old_value = builder.CreateLoad(llvm_value_type, target_pointer);
                llvm::Value* updated_value = builder.CreateAdd(old_value, scalar_one(llvm_value_type));
                builder.CreateStore(updated_value, target_pointer);
                if (quxlang::is_ref(state.routine->local_types.at(local_slot_index(instruction.target2)).type))
                {
                    store_reference_pointer(state, builder, instruction.target2, target_pointer);
                }
                else
                {
                    store_slot_value(state, builder, instruction.target2, updated_value);
                }
                return;
            }
            if (quxlang::is_ptr(pointee_type))
            {
                quxlang::type_symbol element_type = quxlang::remove_ptr(pointee_type);
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::getSigned(i64_type(), static_cast< std::int64_t >(slot_size(element_type))));
                builder.CreateStore(updated_pointer, target_pointer);
                if (quxlang::is_ref(state.routine->local_types.at(local_slot_index(instruction.target2)).type))
                {
                    store_reference_pointer(state, builder, instruction.target2, target_pointer);
                }
                else
                {
                    store_slot_value(state, builder, instruction.target2, updated_pointer);
                }
                return;
            }
            throw quxlang::semantic_compilation_error("PREINC/PREDEC requires a reference to an integer or pointer, got " + quxlang::to_string(reference_type));
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::predecrement const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& reference_type = state.routine->local_types.at(local_slot_index(instruction.target)).type;
            if (!quxlang::is_ref(reference_type))
            {
                throw quxlang::semantic_compilation_error("PREINC/PREDEC requires a reference slot, got " + quxlang::to_string(reference_type));
            }
            quxlang::type_symbol pointee_type = quxlang::remove_ref(reference_type);
            llvm::Value* target_pointer = load_reference_pointer(state, builder, instruction.target);
            if (pointee_type.type_is< quxlang::int_type >() || pointee_type.type_is< quxlang::bool_type >() || pointee_type.type_is< quxlang::byte_type >() || pointee_type.type_is< quxlang::size_type >())
            {
                llvm::Type* llvm_value_type = value_storage_type(pointee_type);
                llvm::Value* old_value = builder.CreateLoad(llvm_value_type, target_pointer);
                llvm::Value* updated_value = builder.CreateSub(old_value, scalar_one(llvm_value_type));
                builder.CreateStore(updated_value, target_pointer);
                if (quxlang::is_ref(state.routine->local_types.at(local_slot_index(instruction.target2)).type))
                {
                    store_reference_pointer(state, builder, instruction.target2, target_pointer);
                }
                else
                {
                    store_slot_value(state, builder, instruction.target2, updated_value);
                }
                return;
            }
            if (quxlang::is_ptr(pointee_type))
            {
                quxlang::type_symbol element_type = quxlang::remove_ptr(pointee_type);
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::getSigned(i64_type(), -static_cast< std::int64_t >(slot_size(element_type))));
                builder.CreateStore(updated_pointer, target_pointer);
                if (quxlang::is_ref(state.routine->local_types.at(local_slot_index(instruction.target2)).type))
                {
                    store_reference_pointer(state, builder, instruction.target2, target_pointer);
                }
                else
                {
                    store_slot_value(state, builder, instruction.target2, updated_pointer);
                }
                return;
            }
            throw quxlang::semantic_compilation_error("PREINC/PREDEC requires a reference to an integer or pointer, got " + quxlang::to_string(reference_type));
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pointer_arith const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::pointer_arith const& inst = instruction;
            quxlang::type_symbol pointer_type = state.routine->local_types.at(local_slot_index(inst.from)).type;
            quxlang::type_symbol element_type = quxlang::remove_ptr(pointer_type);
            quxlang::type_symbol const& offset_type = state.routine->local_types.at(local_slot_index(inst.offset)).type;
            bool const signed_offset = offset_type.type_is< quxlang::int_type >() && offset_type.get_as< quxlang::int_type >().has_sign;
            llvm::Value* base_pointer = load_slot_value(state, builder, inst.from);
            llvm::Value* index_value = integer_value(state, builder, inst.offset);
            llvm::IntegerType* const index_type = llvm::cast< llvm::IntegerType >(index_value->getType());
            if (index_type != i64_type())
            {
                if (index_type->getBitWidth() > i64_type()->getBitWidth())
                {
                    index_value = builder.CreateTrunc(index_value, i64_type());
                }
                else if (signed_offset)
                {
                    index_value = builder.CreateSExt(index_value, i64_type());
                }
                else
                {
                    index_value = builder.CreateZExt(index_value, i64_type());
                }
            }
            if (inst.multiplier == -1)
            {
                index_value = builder.CreateNeg(index_value);
            }
            else if (inst.multiplier != 1)
            {
                throw quxlang::semantic_compilation_error("PTR_ARITH multiplier must be 1 or -1 for LLVM lowering");
            }
            llvm::Value* byte_pointer = builder.CreateBitCast(base_pointer, opaque_pointer_type());
            llvm::Value* byte_offset = builder.CreateMul(index_value, llvm::ConstantInt::get(i64_type(), slot_size(element_type)));
            llvm::Value* element_pointer = builder.CreateInBoundsGEP(i8_type(), byte_pointer, byte_offset);
            store_slot_value(state, builder, inst.result, element_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pointer_diff const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::pointer_diff const& inst = instruction;
            quxlang::type_symbol const& from_type = state.routine->local_types.at(local_slot_index(inst.from)).type;
            quxlang::type_symbol const& to_type = state.routine->local_types.at(local_slot_index(inst.to)).type;
            if (!quxlang::is_ptr(from_type) || !quxlang::is_ptr(to_type))
            {
                throw quxlang::semantic_compilation_error("PTR_DIFF requires pointer operands for LLVM lowering");
            }

            quxlang::type_symbol const element_type = quxlang::remove_ptr(from_type);
            if (element_type != quxlang::remove_ptr(to_type))
            {
                throw quxlang::semantic_compilation_error("PTR_DIFF requires matching pointer element types for LLVM lowering");
            }

            llvm::Value* from_pointer = load_slot_value(state, builder, inst.from);
            llvm::Value* to_pointer = load_slot_value(state, builder, inst.to);
            llvm::Value* from_address = builder.CreatePtrToInt(from_pointer, i64_type());
            llvm::Value* to_address = builder.CreatePtrToInt(to_pointer, i64_type());
            llvm::Value* element_delta = builder.CreateSub(from_address, to_address);
            std::uint64_t const element_size = slot_size(element_type);
            if (element_size != 1)
            {
                element_delta = builder.CreateExactSDiv(element_delta, llvm::ConstantInt::getSigned(i64_type(), static_cast< std::int64_t >(element_size)));
            }

            quxlang::type_symbol const& result_type = state.routine->local_types.at(local_slot_index(inst.result)).type;
            llvm::IntegerType* const destination_type = llvm::cast< llvm::IntegerType >(value_storage_type(result_type));
            if (destination_type != i64_type())
            {
                if (destination_type->getBitWidth() < i64_type()->getBitWidth())
                {
                    element_delta = builder.CreateTrunc(element_delta, destination_type);
                }
                else
                {
                    element_delta = builder.CreateSExt(element_delta, destination_type);
                }
            }
            store_slot_value(state, builder, inst.result, element_delta);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::assert_instr const& instruction)
        {
            quxlang::vmir2::assert_instr const& inst = instruction;
            llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context, "assert.cont", state.function);
            llvm::BasicBlock* fail_block = llvm::BasicBlock::Create(context, "assert.fail", state.function);
            builder.CreateCondBr(truth_value(state, builder, inst.condition), continue_block, fail_block);

            builder.SetInsertPoint(fail_block);
            quxlang::vmir2::source_index const empty_source_index;
            quxlang::vmir2::source_index const& source_index = input.source_index.has_value() ? input.source_index->get() : empty_source_index;
            quxlang::llvm_backend::runtime_procedure_reference const fail_reference{.procedure = quxlang::llvm_backend::runtime_procedure::assert_fail};
            quxlang::type_symbol const& fail_symbol = runtime_procedure_symbol(fail_reference);
            if (!fail_symbol.type_is< quxlang::instanciation_reference >())
            {
                throw quxlang::semantic_compilation_error("Runtime ASSERT_FAIL did not initialize to a concrete procedure: " + quxlang::to_string(fail_symbol));
            }
            callable_abi const fail_abi = callable_abi_from_instanciation_reference(fail_symbol.get_as< quxlang::instanciation_reference >(), std::nullopt);
            llvm::Function* const fail_function = get_or_create_external_function(fail_symbol, fail_abi);
            quxlang::llvm_backend::runtime_assert_fail_call_arguments const fail_arguments = quxlang::llvm_backend::runtime_assert_fail_arguments(inst, source_index);
            llvm::CallInst* const call = builder.CreateCall(fail_abi.llvm_type, fail_function, runtime_assert_fail_call_arguments(fail_arguments, fail_abi));
            apply_calling_convention(call, fail_abi);
            builder.CreateUnreachable();

            current_block = continue_block;
            builder.SetInsertPoint(current_block);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::swap const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::swap const& inst = instruction;
            quxlang::type_symbol const& a_type = state.routine->local_types.at(local_slot_index(inst.a)).type;
            quxlang::type_symbol const& b_type = state.routine->local_types.at(local_slot_index(inst.b)).type;
            if (quxlang::is_ref(a_type) && quxlang::is_ref(b_type))
            {
                quxlang::type_symbol const a_pointee_type = quxlang::remove_ref(a_type);
                quxlang::type_symbol const b_pointee_type = quxlang::remove_ref(b_type);
                llvm::Type* a_storage_type = value_storage_type(a_pointee_type);
                llvm::Type* b_storage_type = value_storage_type(b_pointee_type);
                if (a_storage_type != b_storage_type)
                {
                    throw quxlang::semantic_compilation_error("SWAP on references requires matching pointee storage types for LLVM lowering");
                }

                llvm::Value* a_pointer = load_reference_pointer(state, builder, inst.a);
                llvm::Value* b_pointer = load_reference_pointer(state, builder, inst.b);
                llvm::Value* a_value = builder.CreateLoad(a_storage_type, a_pointer);
                llvm::Value* b_value = builder.CreateLoad(a_storage_type, b_pointer);
                builder.CreateStore(b_value, a_pointer);
                builder.CreateStore(a_value, b_pointer);
                return;
            }

            llvm::Value* a_value = load_slot_value(state, builder, inst.a);
            llvm::Value* b_value = load_slot_value(state, builder, inst.b);
            store_slot_value(state, builder, inst.a, b_value);
            store_slot_value(state, builder, inst.b, a_value);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::unimplemented const& instruction)
        {
            llvm::BasicBlock* unreachable_continue = llvm::BasicBlock::Create(context, "unimplemented.cont", state.function);
            llvm::Function* trap = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::trap);
            builder.CreateCall(trap);
            builder.CreateUnreachable();
            current_block = unreachable_continue;
            builder.SetInsertPoint(current_block);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::array_init_start const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::array_init_start const& inst = instruction;
            quxlang::type_symbol initializer_type = state.routine->local_types.at(local_slot_index(inst.initializer)).type;
            quxlang::array_initializer_type const& array_info = initializer_type.get_as< quxlang::array_initializer_type >();
            llvm::Value* base_pointer = value_address(state, inst.on_value);
            llvm::Value* init_storage = state.locals.at(local_slot_index(inst.initializer)).storage;
            llvm::Value* base_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 0);
            llvm::Value* index_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 1);
            llvm::Value* count_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 2);
            builder.CreateStore(builder.CreateBitCast(base_pointer, opaque_pointer_type()), base_field);
            builder.CreateStore(llvm::ConstantInt::get(i64_type(), 0), index_field);
            builder.CreateStore(llvm::ConstantInt::get(i64_type(), array_info.count), count_field);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::array_init_index const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::array_init_index const& inst = instruction;
            quxlang::type_symbol initializer_type = state.routine->local_types.at(local_slot_index(inst.initializer)).type;
            llvm::Value* init_storage = state.locals.at(local_slot_index(inst.initializer)).storage;
            llvm::Value* index_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 1);
            llvm::Value* index_value = builder.CreateLoad(i64_type(), index_field);
            store_slot_value(state, builder, inst.result, index_value);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::array_init_element const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::array_init_element const& inst = instruction;
            quxlang::type_symbol initializer_type = state.routine->local_types.at(local_slot_index(inst.initializer)).type;
            quxlang::array_initializer_type const& array_info = initializer_type.get_as< quxlang::array_initializer_type >();
            llvm::Value* init_storage = state.locals.at(local_slot_index(inst.initializer)).storage;
            llvm::Value* base_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 0);
            llvm::Value* index_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 1);
            llvm::Value* base_value = builder.CreateLoad(opaque_pointer_type(), base_field);
            llvm::Value* index_value = builder.CreateLoad(i64_type(), index_field);
            llvm::Value* byte_offset = builder.CreateMul(index_value, llvm::ConstantInt::get(i64_type(), slot_size(array_info.element_type)));
            llvm::Value* element_pointer = builder.CreateInBoundsGEP(i8_type(), base_value, byte_offset);
            assign_slot_alias(state, inst.target, element_pointer);
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::array_init_finish const& instruction)
        {
            (void)current_block;
            return;
        }


        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::array_init_more const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::array_init_more const& inst = instruction;
            quxlang::type_symbol initializer_type = state.routine->local_types.at(local_slot_index(inst.initializer)).type;
            llvm::Value* init_storage = state.locals.at(local_slot_index(inst.initializer)).storage;
            llvm::Value* index_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 1);
            llvm::Value* count_field = builder.CreateStructGEP(value_storage_type(initializer_type), init_storage, 2);
            llvm::Value* index_value = builder.CreateLoad(i64_type(), index_field);
            llvm::Value* count_value = builder.CreateLoad(i64_type(), count_field);
            store_boolean(state, builder, inst.result, builder.CreateICmpULT(index_value, count_value));
            return;
        }



        void emit_terminator(function_codegen_state& state, llvm::BasicBlock* current_block, quxlang::vmir2::vm_terminator const& terminator)
        {
            builder.SetInsertPoint(current_block);
            apply_debug_location(state, quxlang::vmir2::get_location(terminator));
            if (terminator.type_is< quxlang::vmir2::jump >())
            {
                quxlang::vmir2::jump const& inst = terminator.as< quxlang::vmir2::jump >();
                llvm::BasicBlock* target_block = state.blocks.at(inst.target);
                llvm::BasicBlock* edge_target = cleanup_edge_target(
                    state,
                    current_block,
                    state.current_state,
                    state.routine->blocks.at(block_slot_index(inst.target)).entry_state,
                    target_block);
                builder.CreateBr(edge_target);
                return;
            }
            if (terminator.type_is< quxlang::vmir2::branch >())
            {
                quxlang::vmir2::branch const& inst = terminator.as< quxlang::vmir2::branch >();
                llvm::BasicBlock* true_target = cleanup_edge_target(
                    state,
                    current_block,
                    state.current_state,
                    state.routine->blocks.at(block_slot_index(inst.target_true)).entry_state,
                    state.blocks.at(inst.target_true));
                llvm::BasicBlock* false_target = cleanup_edge_target(
                    state,
                    current_block,
                    state.current_state,
                    state.routine->blocks.at(block_slot_index(inst.target_false)).entry_state,
                    state.blocks.at(inst.target_false));
                builder.CreateCondBr(truth_value(state, builder, inst.condition), true_target, false_target);
                return;
            }
            if (terminator.type_is< quxlang::vmir2::ret >())
            {
                emit_return_cleanup(state, builder, state.current_state);
                if (state.abi != nullptr && state.abi->return_source_index.has_value())
                {
                    quxlang::vmir2::local_index const return_slot = source_argument_slot(*state.abi, routine_parameter_invocation_args(state), *state.abi->return_source_index);
                    builder.CreateRet(load_slot_value(state, builder, return_slot));
                }
                else
                {
                    builder.CreateRetVoid();
                }
                return;
            }
            if (terminator.type_is< quxlang::vmir2::runtime_constexpr >())
            {
                quxlang::vmir2::runtime_constexpr const& inst = terminator.as< quxlang::vmir2::runtime_constexpr >();
                llvm::BasicBlock* target_block = state.blocks.at(inst.target_native);
                llvm::BasicBlock* edge_target = cleanup_edge_target(
                    state,
                    current_block,
                    state.current_state,
                    state.routine->blocks.at(block_slot_index(inst.target_native)).entry_state,
                    target_block);
                builder.CreateBr(edge_target);
                return;
            }
            if (terminator.type_is< quxlang::vmir2::initguard_try_acquire >())
            {
                quxlang::vmir2::initguard_try_acquire const& inst = terminator.as< quxlang::vmir2::initguard_try_acquire >();
                llvm::Value* guard_pointer = get_or_create_initguard_global(inst.symbol, inst.class_);
                callable_abi const try_acquire_abi = initguard_runtime_abi(quxlang::llvm_backend::runtime_procedure::initguard_try_acquire);
                llvm::Function* try_acquire = get_or_create_initguard_runtime_function(quxlang::llvm_backend::runtime_procedure::initguard_try_acquire, try_acquire_abi);
                llvm::CallInst* try_acquire_call = builder.CreateCall(try_acquire_abi.llvm_type, try_acquire, {guard_pointer});
                apply_calling_convention(try_acquire_call, try_acquire_abi);
                llvm::Value* acquired = builder.CreateICmpNE(
                    try_acquire_call,
                    llvm::ConstantInt::get(llvm::cast< llvm::IntegerType >(try_acquire_call->getType()), 0));
                llvm::BasicBlock* acquired_block = state.blocks.at(inst.target_acquired);
                llvm::BasicBlock* initialized_block = state.blocks.at(inst.target_already_initialized);
                llvm::BasicBlock* initialized_edge = cleanup_edge_target(
                    state,
                    current_block,
                    state.current_state,
                    state.routine->blocks.at(block_slot_index(inst.target_already_initialized)).entry_state,
                    initialized_block);

                quxlang::vmir2::state_map acquired_state = state.current_state;
                acquired_state[inst.target_lock].stage = quxlang::vmir2::slot_stage::full;
                acquired_state[inst.target_lock].storage_valid = true;
                llvm::BasicBlock* acquired_store_block = llvm::BasicBlock::Create(context, "initguard.acquired", state.function);
                llvm::BasicBlock* acquired_edge = cleanup_edge_target(
                    state,
                    acquired_store_block,
                    acquired_state,
                    state.routine->blocks.at(block_slot_index(inst.target_acquired)).entry_state,
                    acquired_block);
                builder.CreateCondBr(acquired, acquired_store_block, initialized_edge);

                builder.SetInsertPoint(acquired_store_block);
                store_slot_value(state, builder, inst.target_lock, builder.CreateBitCast(guard_pointer, opaque_pointer_type()));
                builder.CreateBr(acquired_edge);
                return;
            }

            quxlang::vmir2::assembler assembler(*state.routine);
            throw quxlang::semantic_compilation_error("Unsupported VMIR2 terminator for LLVM lowering: " + assembler.to_string(terminator));
        }

        void emit_defined_function(quxlang::type_symbol const& symbol, quxlang::vmir2::functanoid_routine3 const& routine)
        {
            llvm::Function* function = functions.at(symbol);
            if (!function->empty())
            {
                return;
            }

            if (debug_builder)
            {
                function->setSubprogram(debug_subprogram(symbol, routine));
            }

            function_codegen_state state;
            state.function = function;
            state.routine = &routine;
            state.abi = &function_abis.at(symbol);
            state.locals.resize(routine.local_types.size());

            llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(context, "entry", function);
            llvm::IRBuilder<> prologue(entry_block);
            builder.SetCurrentDebugLocation(llvm::DebugLoc());
            std::vector< bool > native_reachable_blocks(routine.blocks.size(), false);
            if (!routine.blocks.empty())
            {
                std::vector< quxlang::vmir2::block_index > pending_blocks;
                native_reachable_blocks[0] = true;
                pending_blocks.push_back(quxlang::vmir2::block_index(0));
                while (!pending_blocks.empty())
                {
                    quxlang::vmir2::block_index const block_index = pending_blocks.back();
                    pending_blocks.pop_back();
                    quxlang::vmir2::executable_block const& block = routine.blocks.at(block_slot_index(block_index));
                    if (!block.terminator.has_value())
                    {
                        continue;
                    }

                    auto enqueue_block = [&](quxlang::vmir2::block_index const target)
                    {
                        if (native_reachable_blocks[block_slot_index(target)])
                        {
                            return;
                        }
                        native_reachable_blocks[block_slot_index(target)] = true;
                        pending_blocks.push_back(target);
                    };

                    quxlang::vmir2::vm_terminator const& terminator = *block.terminator;
                    if (terminator.type_is< quxlang::vmir2::jump >())
                    {
                        enqueue_block(terminator.as< quxlang::vmir2::jump >().target);
                    }
                    else if (terminator.type_is< quxlang::vmir2::branch >())
                    {
                        quxlang::vmir2::branch const& branch = terminator.as< quxlang::vmir2::branch >();
                        enqueue_block(branch.target_true);
                        enqueue_block(branch.target_false);
                    }
                    else if (terminator.type_is< quxlang::vmir2::runtime_constexpr >())
                    {
                        enqueue_block(terminator.as< quxlang::vmir2::runtime_constexpr >().target_native);
                    }
                    else if (terminator.type_is< quxlang::vmir2::initguard_try_acquire >())
                    {
                        quxlang::vmir2::initguard_try_acquire const& try_acquire = terminator.as< quxlang::vmir2::initguard_try_acquire >();
                        enqueue_block(try_acquire.target_acquired);
                        enqueue_block(try_acquire.target_already_initialized);
                    }
                }
            }

            std::vector< routine_abi_parameter > const params = ordered_routine_parameters(routine);
            std::map< std::size_t, bool > caller_provided_slots;
            for (std::size_t source_index = 0; source_index < params.size(); ++source_index)
            {
                if (state.abi->return_source_index.has_value() && *state.abi->return_source_index == source_index)
                {
                    continue;
                }

                routine_abi_parameter const& param = params[source_index];
                if (is_output_slot_type(param.parameter_type) || !abi_passes_by_value(param.parameter_type))
                {
                    caller_provided_slots[local_slot_index(param.local)] = true;
                }
            }

            for (std::size_t i = 0; i < routine.local_types.size(); ++i)
            {
                if (caller_provided_slots.contains(i))
                {
                    continue;
                }
                if (routine.local_types[i].type.type_is< quxlang::void_type >())
                {
                    continue;
                }
                llvm::Type* storage_type = value_storage_type(routine.local_types[i].type);
                state.locals[i].storage = prologue.CreateAlloca(storage_type, nullptr, "slot" + std::to_string(i));
                llvm::cast< llvm::AllocaInst >(state.locals[i].storage)->setAlignment(llvm::Align(slot_alignment(routine.local_types[i].type)));
            }

            llvm::Function::arg_iterator arg_iter = function->arg_begin();
            for (std::size_t source_index = 0; source_index < params.size(); ++source_index)
            {
                routine_abi_parameter const& param = params[source_index];
                if (state.abi->return_source_index.has_value() && *state.abi->return_source_index == source_index)
                {
                    continue;
                }
                llvm::Argument& arg = *arg_iter++;
                std::string const arg_name = routine_argument_name(param);
                if (is_output_slot_type(param.parameter_type))
                {
                    arg.setName(arg_name);
                    state.locals.at(local_slot_index(param.local)).storage = &arg;
                    continue;
                }
                if (!abi_passes_by_value(param.parameter_type))
                {
                    arg.setName(arg_name);
                    state.locals.at(local_slot_index(param.local)).storage = &arg;
                    continue;
                }
                arg.setName(arg_name);
                prologue.CreateStore(&arg, state.locals.at(local_slot_index(param.local)).storage);
            }

            for (std::size_t i = 0; i < routine.blocks.size(); ++i)
            {
                if (!native_reachable_blocks[i])
                {
                    continue;
                }
                std::string block_name = "block" + std::to_string(i);
                std::map< quxlang::vmir2::block_index, std::string >::const_iterator name_iter = routine.block_names.find(quxlang::vmir2::block_index(i));
                if (name_iter != routine.block_names.end())
                {
                    block_name = name_iter->second;
                }
                state.blocks.emplace(quxlang::vmir2::block_index(i), llvm::BasicBlock::Create(context, block_name, function));
            }

            prologue.CreateBr(state.blocks.at(quxlang::vmir2::block_index(0)));

            for (std::size_t block_i = 0; block_i < routine.blocks.size(); ++block_i)
            {
                if (!native_reachable_blocks[block_i])
                {
                    continue;
                }
                quxlang::vmir2::block_index block_index(block_i);
                llvm::BasicBlock* current_block = state.blocks.at(block_index);
                builder.SetInsertPoint(current_block);

                quxlang::vmir2::executable_block const& block = routine.blocks.at(block_i);
                state.current_state = block.entry_state;
                if (block_i == 0 && state.current_state.empty())
                {
                    quxlang::vmir2::codegen_state_engine entry_engine(state.current_state, routine.local_types, routine.parameters);
                    entry_engine.apply_entry();
                }
                quxlang::vmir2::codegen_state_engine state_engine(state.current_state, routine.local_types, routine.parameters);
                for (quxlang::vmir2::vm_instruction const& inst : block.instructions)
                {
                    quxlang::vmir2::state_map const previous_state = state.current_state;
                    begin_vmir2_metadata_context(vmir2_metadata_text(routine, inst));
                    emit_instruction(state, current_block, inst);
                    state_engine.apply(inst);
                    emit_post_instruction_array_initializer_progress(state, builder, previous_state, state.current_state);
                    emit_post_instruction_poison_cleanup(state, builder, previous_state, state.current_state, inst);
                    end_vmir2_metadata_context();
                }

                if (current_block->getTerminator() != nullptr)
                {
                    continue;
                }

                if (block.terminator.has_value())
                {
                    begin_vmir2_metadata_context(vmir2_metadata_text(routine, *block.terminator));
                    emit_terminator(state, current_block, *block.terminator);
                    end_vmir2_metadata_context();
                }
                else
                {
                    builder.SetInsertPoint(current_block);
                    if (state.abi != nullptr && state.abi->llvm_type->getReturnType()->isVoidTy())
                    {
                        builder.CreateRetVoid();
                    }
                    else
                    {
                        builder.CreateUnreachable();
                    }
                }
            }
        }

        /**
         * Emits a defined VMIR routine and annotates lowering diagnostics with the routine symbol.
         */
        void emit_defined_function_with_traceback(quxlang::type_symbol const& symbol, quxlang::vmir2::functanoid_routine3 const& routine)
        {
            try
            {
                emit_defined_function(symbol, routine);
            }
            catch (quxlang::compilation_error& error)
            {
                error.traceback.push_back(quxlang::trace_frame{
                    .trace_context = "lowering routine " + quxlang::to_string(symbol),
                    .location = std::nullopt,
                });
                throw;
            }
        }
    };
} // namespace quxlang::llvm_backend::detail

auto quxlang::llvm_backend::llvm_backend::compile(quxlang::llvm_backend::llvm_compilable_unit const& input) const -> quxlang::llvm_backend::llvm_compiled_unit
{
    detail::llvm_module_codegen codegen(input);
    return codegen.compile();
}

auto quxlang::llvm_backend::llvm_backend::assemble(
    quxlang::llvm_backend::llvm_compilation_target const& target,
    quxlang::asm_procedure const& procedure) const -> quxlang::llvm_backend::llvm_assembled_procedure
{
    auto assembly_text = [&]() -> std::string
    {
        if (procedure.architecture == "ARM")
        {
            return quxlang::convert_to_arm_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name);
        }
        if (procedure.architecture == "X64")
        {
            return quxlang::convert_to_x64_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name);
        }
        throw quxlang::semantic_compilation_error("Unsupported asm procedure architecture for LLVM lowering: " + procedure.architecture);
    }();

    switch (target.machine.cpu_type)
    {
    case quxlang::cpu::x86_32:
    case quxlang::cpu::x86_64:
    {
        static bool const initialized = []() -> bool
        {
            ::LLVMInitializeX86TargetInfo();
            ::LLVMInitializeX86Target();
            ::LLVMInitializeX86TargetMC();
            ::LLVMInitializeX86AsmParser();
            ::LLVMInitializeX86AsmPrinter();
            return true;
        }();
        (void)initialized;
        break;
    }
    case quxlang::cpu::arm_32:
    {
        static bool const initialized = []() -> bool
        {
            ::LLVMInitializeARMTargetInfo();
            ::LLVMInitializeARMTarget();
            ::LLVMInitializeARMTargetMC();
            ::LLVMInitializeARMAsmParser();
            ::LLVMInitializeARMAsmPrinter();
            return true;
        }();
        (void)initialized;
        break;
    }
    case quxlang::cpu::arm_64:
    {
        static bool const initialized = []() -> bool
        {
            ::LLVMInitializeAArch64TargetInfo();
            ::LLVMInitializeAArch64Target();
            ::LLVMInitializeAArch64TargetMC();
            ::LLVMInitializeAArch64AsmParser();
            ::LLVMInitializeAArch64AsmPrinter();
            return true;
        }();
        (void)initialized;
        break;
    }
    case quxlang::cpu::riscv_32:
    case quxlang::cpu::riscv_64:
    {
        static bool const initialized = []() -> bool
        {
            ::LLVMInitializeRISCVTargetInfo();
            ::LLVMInitializeRISCVTarget();
            ::LLVMInitializeRISCVTargetMC();
            ::LLVMInitializeRISCVAsmParser();
            ::LLVMInitializeRISCVAsmPrinter();
            return true;
        }();
        (void)initialized;
        break;
    }
    case quxlang::cpu::none:
        throw quxlang::semantic_compilation_error("Unsupported LLVM target initialization CPU kind");
    }

    std::string const triple_text = quxlang::lookup_llvm_triple(target.machine);
    ::llvm::Triple triple(triple_text);
    std::string target_error;
    ::llvm::Target const* llvm_target = ::llvm::TargetRegistry::lookupTarget(triple, target_error);
    if (llvm_target == nullptr)
    {
        throw quxlang::semantic_compilation_error("Failed to lookup LLVM target for " + triple_text + ": " + target_error);
    }

    ::llvm::SourceMgr source_manager;
    source_manager.AddNewSourceBuffer(::llvm::MemoryBuffer::getMemBufferCopy(assembly_text), ::llvm::SMLoc());

    ::llvm::MCTargetOptions mc_options;
    std::unique_ptr< ::llvm::MCSubtargetInfo > subtarget_info(llvm_target->createMCSubtargetInfo(triple, "generic", ""));
    std::unique_ptr< ::llvm::MCRegisterInfo > register_info(llvm_target->createMCRegInfo(triple));
    if (!subtarget_info || !register_info)
    {
        throw quxlang::semantic_compilation_error("Failed to create LLVM MC target state for " + triple_text);
    }

    std::unique_ptr< ::llvm::MCAsmInfo > asm_info(llvm_target->createMCAsmInfo(*register_info, triple, mc_options));
    if (!asm_info)
    {
        throw quxlang::semantic_compilation_error("Failed to create LLVM MC asm info for " + triple_text);
    }

    ::llvm::MCContext machine_context(triple, asm_info.get(), register_info.get(), subtarget_info.get(), &source_manager, &mc_options);
    std::unique_ptr< ::llvm::MCObjectFileInfo > object_file_info(llvm_target->createMCObjectFileInfo(machine_context, false, true));
    machine_context.setObjectFileInfo(object_file_info.get());

    std::unique_ptr< ::llvm::MCAsmBackend > asm_backend(llvm_target->createMCAsmBackend(*subtarget_info, *register_info, mc_options));
    std::unique_ptr< ::llvm::MCInstrInfo > instruction_info(llvm_target->createMCInstrInfo());
    if (!asm_backend || !instruction_info)
    {
        throw quxlang::semantic_compilation_error("Failed to create LLVM MC backend for " + triple_text);
    }

    ::llvm::SmallVector< char, 0 > object_buffer;
    auto object_stream = std::make_unique< ::llvm::raw_svector_ostream >(object_buffer);
    std::unique_ptr< ::llvm::MCObjectWriter > object_writer(asm_backend->createObjectWriter(*object_stream));
    std::unique_ptr< ::llvm::MCCodeEmitter > code_emitter(llvm_target->createMCCodeEmitter(*instruction_info, machine_context));
    if (!object_writer || !code_emitter)
    {
        throw quxlang::semantic_compilation_error("Failed to create LLVM MC object writer for " + triple_text);
    }

    std::unique_ptr< ::llvm::MCStreamer > streamer(
        llvm_target->createMCObjectStreamer(triple, machine_context, std::move(asm_backend), std::move(object_writer), std::move(code_emitter), *subtarget_info));
    if (!streamer)
    {
        throw quxlang::semantic_compilation_error("Failed to create LLVM MC object streamer for " + triple_text);
    }

    ::llvm::MCAsmParser* parser = ::llvm::createMCAsmParser(source_manager, machine_context, *streamer, *asm_info);
    std::unique_ptr< ::llvm::MCTargetAsmParser > target_parser(llvm_target->createMCAsmParser(*subtarget_info, *parser, *instruction_info, mc_options));
    if (!target_parser)
    {
        throw quxlang::semantic_compilation_error("Failed to create LLVM MC asm parser for " + triple_text);
    }
    parser->setTargetParser(*target_parser);
    if (parser->Run(false))
    {
        throw quxlang::semantic_compilation_error("LLVM MC assembly parsing failed for " + procedure.name);
    }

    quxlang::llvm_backend::llvm_assembled_procedure result;
    result.assembly_text = std::move(assembly_text);
    result.object_file.resize(object_buffer.size());
    for (std::size_t i = 0; i < object_buffer.size(); i++)
    {
        result.object_file[i] = static_cast< std::byte >(object_buffer[i]);
    }
    return result;
}
