// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#include <quxlang/llvm-backend.hpp>

#include <quxlang/backends/asm/gnu_asm_converter.hpp>
#include <quxlang/backends/asm/x64_asm_converter.hpp>
#include <quxlang/bytemath.hpp>
#include <quxlang/cpu_attributes.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/manipulators/llvm_lookup.hpp>
#include <quxlang/manipulators/numeric_literal_utils.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_int.hpp>
#include <quxlang/vmir2/assembler.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>
#include <quxlang/vmir2/state_engine.hpp>

#include "llvm_backend_internal.hpp"

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
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
#include <llvm/Support/Error.h>
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
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace quxlang::llvm_backend::detail
{
    /// Rejects enum metadata that does not contain an exact canonical fixed-width representation.
    void require_canonical_enum_value(quxlang::enum_info const& info, std::vector< std::byte > const& value)
    {
        if (info.format.bit_width == 0 || value.size() != info.format.storage_bytes())
        {
            throw quxlang::compiler_bug("ENUM case has malformed canonical byte width");
        }
        std::uint64_t const final_bits = info.format.bit_width % 8;
        if (final_bits != 0)
        {
            std::uint8_t const allowed = static_cast< std::uint8_t >((std::uint16_t{1} << final_bits) - 1);
            if ((std::to_integer< std::uint8_t >(value.back()) & static_cast< std::uint8_t >(~allowed)) != 0)
            {
                throw quxlang::compiler_bug("ENUM case has noncanonical high padding bits");
            }
        }
    }

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

    /** One exact-offset LLVM constant embedded in aggregate storage. */
    struct constant_storage_segment
    {
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
        llvm::Constant* value = nullptr;
    };

    /** Resolved address and semantic type of one nested antestatal object. */
    struct resolved_antestatal_object
    {
        llvm::Constant* pointer = nullptr;
        quxlang::type_symbol type;
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
        std::map< quxlang::vmir2::local_index, bool > fixed_cpu_attribute_references;
    };

    class llvm_module_codegen
    {
    public:
        explicit llvm_module_codegen(quxlang::llvm_backend::llvm_compilable_unit const& input_packet) :
            input(input_packet),
              module(std::make_unique< llvm::Module >(quxlang::to_string(input_packet.target_name), context)),
            builder(context),
              target_machine(create_target_machine(input_packet.machine_target))
        {
            module->setTargetTriple(llvm::Triple(quxlang::lookup_llvm_triple(input.machine_target.machine)));
            module->setDataLayout(target_machine->createDataLayout());
            module->setSourceFileName(primary_source_filename());
            unsigned const type_index_bits = static_cast< unsigned >(input.machine_target.machine.pointer_size_bytes() * 8);
            if (!input.type_index_ordinals.empty() && type_index_bits < 64 && input.type_index_ordinals.rbegin()->second >= (std::uint64_t{1} << type_index_bits))
            {
                throw quxlang::lowering_compilation_error("TYPE_INDEX ordinal does not fit the target pointer-sized representation");
            }
            if (input.source_index.has_value())
            {
                module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
                module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 5U);
                debug_builder = std::make_unique< llvm::DIBuilder >(*module);

                llvm::DIFile* const compile_file = default_debug_file();
                debug_compile_unit = debug_builder->createCompileUnit(llvm::dwarf::DW_LANG_C_plus_plus, compile_file, "qxc", input.machine_target.optimization == quxlang::llvm_backend::optimization_level::release, "", 0);
            }
        }

        auto preoptimize() -> quxlang::llvm_backend::llvm_preoptimized_unit
        {
            generate_module();
            verify_generated_module();

            quxlang::llvm_backend::llvm_preoptimized_unit result;
            result.bitcode = module_bitcode(*module);
            result.llvm_ir_text = module_ir_text();
            result.source_filename = module->getSourceFileName();
            result.target = input.machine_target;
            return result;
        }

    private:
        /** Populates the LLVM module with every declaration, definition, global, and entrypoint in the input packet. */
        void generate_module()
        {
            for (std::pair< quxlang::type_symbol const, quxlang::asm_callable > const& callable_entry : input.asm_callable_interfaces)
            {
                declare_asm_callable_function(callable_entry.first, callable_entry.second);
            }
            if (!input.asm_functions.contains(input.target_name))
            {
                llvm::GlobalValue::LinkageTypes target_linkage = input.root_routine == quxlang::llvm_backend::root_routine_emission::external_declaration ? llvm::GlobalValue::ExternalLinkage : input.whole_module && input.machine_target.machine.binary_type == quxlang::binary::pe && !input.definitions_are_coalescible ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::LinkOnceODRLinkage;
                declare_defined_function(input.target_name, input.target_code, target_linkage);
            }
            for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& function_entry : input.inlinable_functions)
            {
                if (input.asm_functions.contains(function_entry.first))
                {
                    continue;
                }
                llvm::GlobalValue::LinkageTypes routine_linkage;
                if (input.suffix_generated_function_symbols && is_unit_test_procedure(function_entry.first))
                {
                    routine_linkage = llvm::GlobalValue::ExternalLinkage;
                }
                else if (!input.whole_module)
                {
                    routine_linkage = llvm::GlobalValue::AvailableExternallyLinkage;
                }
                else if (input.machine_target.machine.binary_type == quxlang::binary::pe && !input.definitions_are_coalescible)
                {
                    routine_linkage = llvm::GlobalValue::ExternalLinkage;
                }
                else
                {
                    routine_linkage = llvm::GlobalValue::LinkOnceODRLinkage;
                }
                declare_defined_function(function_entry.first, function_entry.second, routine_linkage);
            }
            declare_unit_test_procedures();
            declare_post_detect_procedure();

            emit_cpu_stepping_support();
            emit_object_reference_globals();
            emit_struct_runtime_descriptors();

            if (!input.asm_functions.contains(input.target_name) && input.root_routine == quxlang::llvm_backend::root_routine_emission::definition)
            {
                emit_defined_function_with_traceback(input.target_name, input.target_code);
            }
            for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& function_entry : input.inlinable_functions)
            {
                if (input.asm_functions.contains(function_entry.first))
                {
                    continue;
                }
                emit_defined_function_with_traceback(function_entry.first, function_entry.second);
            }
            if (input.machine_target.machine.binary_type == quxlang::binary::pe)
            {
                for (quxlang::type_symbol const& symbol : input.extern_procedures)
                {
                    if (llvm::Function* function = module->getFunction(symbol_link_name(symbol)))
                    {
                        function->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
                    }
                }
            }
            for (std::pair< quxlang::type_symbol const, quxlang::asm_procedure > const& asm_entry : input.asm_functions)
            {
                if (asm_entry.first == input.target_name && input.root_routine == quxlang::llvm_backend::root_routine_emission::external_declaration)
                {
                    continue;
                }
                module->appendModuleInlineAsm(assembly_text(asm_entry.second));
            }

            if (should_emit_linux_start())
            {
                emit_linux_start();
            }
            if (should_emit_macos_start())
            {
                emit_macos_start();
            }
            if (should_emit_windows_start())
            {
                emit_windows_start();
            }

            suffix_generated_function_symbols();
            apply_function_codegen_configuration();

            if (debug_builder)
            {
                debug_builder->finalize();
            }
        }

        /** Verifies the generated pre-optimization LLVM module. */
        void verify_generated_module() const
        {
            std::string verify_error;
            llvm::raw_string_ostream verify_stream(verify_error);
            if (llvm::verifyModule(*module, &verify_stream))
            {
                throw quxlang::semantic_compilation_error("LLVM IR verification failed for " + quxlang::to_string(input.target_name) + ": " + verify_stream.str());
            }
        }

        /** Serializes the generated LLVM module as textual IR. */
        auto module_ir_text() const -> std::string
        {
            return module_ir_text(*module);
        }

    public:
        /** Serializes one LLVM module as textual IR. */
        static auto module_ir_text(llvm::Module const& source_module) -> std::string
        {
            std::string result;
            llvm::raw_string_ostream ir_stream(result);
            source_module.print(ir_stream, nullptr);
            return result;
        }

        /** Serializes one LLVM module in the binary bitcode format. */
        static auto module_bitcode(llvm::Module const& source_module) -> std::vector< std::byte >
        {
            llvm::SmallVector< char, 0 > bitcode_buffer;
            llvm::raw_svector_ostream bitcode_stream(bitcode_buffer);
            llvm::WriteBitcodeToFile(source_module, bitcode_stream);
            std::vector< std::byte > result(bitcode_buffer.size());
            for (std::size_t i = 0; i < bitcode_buffer.size(); ++i)
            {
                result[i] = static_cast< std::byte >(bitcode_buffer[i]);
            }
            return result;
        }

        /** Clones and optimizes one LLVM module while preserving all generated definitions. */
        static auto optimize_module(llvm::Module const& source_module, llvm::TargetMachine* target_machine) -> std::unique_ptr< llvm::Module >
        {
            std::unique_ptr< llvm::Module > optimized_module = llvm::CloneModule(source_module);
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

            llvm::PassBuilder pass_builder(target_machine);
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
                throw quxlang::semantic_compilation_error("Optimized LLVM IR verification failed for " + source_module.getModuleIdentifier() + ": " + verify_stream.str());
            }

            if (llvm::GlobalVariable* used = optimized_module->getGlobalVariable("llvm.used"))
            {
                used->eraseFromParent();
            }
            return optimized_module;
        }

        /** Parses one query-stage LLVM bitcode result into a module owned by context. */
        static auto parse_module_bitcode(std::vector< std::byte > const& bitcode, llvm::LLVMContext& context, std::string_view stage_name) -> std::unique_ptr< llvm::Module >
        {
            llvm::StringRef input_data(reinterpret_cast< char const* >(bitcode.data()), bitcode.size());
            std::unique_ptr< llvm::MemoryBuffer > input_buffer = llvm::MemoryBuffer::getMemBufferCopy(input_data, std::string(stage_name));
            llvm::Expected< std::unique_ptr< llvm::Module > > module_result = llvm::parseBitcodeFile(input_buffer->getMemBufferRef(), context);
            if (!module_result)
            {
                throw quxlang::semantic_compilation_error("Failed to parse " + std::string(stage_name) + " LLVM bitcode: " + llvm::toString(module_result.takeError()));
            }
            return std::move(*module_result);
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
        std::map< quxlang::struct_phase_descriptor_key, llvm::GlobalVariable* > struct_phase_descriptors;
        std::map< std::pair< quxlang::type_symbol, quxlang::struct_phase_key >, llvm::GlobalVariable* > struct_phase_descriptor_groups;
        llvm::StructType* struct_runtime_descriptor_struct = nullptr;
        llvm::StructType* struct_phase_assignment_struct = nullptr;
        llvm::ArrayType* struct_phase_transition_array = nullptr;
        llvm::StructType* struct_phase_group_struct = nullptr;
        std::size_t struct_phase_assignment_capacity = 1;
        std::unique_ptr< llvm::DIBuilder > debug_builder;
        llvm::DICompileUnit* debug_compile_unit = nullptr;
        std::map< std::uint64_t, llvm::DIFile* > debug_files;
        std::map< quxlang::type_symbol, llvm::DISubprogram* > debug_subprograms;
        std::size_t helper_counter = 0;

        /** Returns the fixed-prefix LLVM representation of one inheritance runtime descriptor. */
        auto struct_runtime_descriptor_type() -> llvm::StructType*
        {
            if (struct_runtime_descriptor_struct == nullptr)
            {
                struct_runtime_descriptor_struct = llvm::StructType::create(context, {
                    pointer_integer_type(),
                    i64_type(),
                    pointer_integer_type(),
                    pointer_integer_type(),
                    opaque_pointer_type(),
                    pointer_integer_type(),
                    opaque_pointer_type(),
                    pointer_integer_type(),
                    opaque_pointer_type(),
                    opaque_pointer_type(),
                }, "qux.struct_runtime_descriptor");
            }
            return struct_runtime_descriptor_struct;
        }

        /** Returns the LLVM representation of one descriptor assignment. */
        auto struct_phase_assignment_type() -> llvm::StructType*
        {
            if (struct_phase_assignment_struct == nullptr)
            {
                struct_phase_assignment_struct = llvm::StructType::create(context, {i64_type(), opaque_pointer_type()}, "qux.struct_phase_assignment");
            }
            return struct_phase_assignment_struct;
        }

        /** Returns the fixed-capacity LLVM representation of one phase transition. */
        auto struct_phase_transition_type() -> llvm::ArrayType*
        {
            if (struct_phase_transition_array == nullptr)
            {
                struct_phase_transition_array = llvm::ArrayType::get(struct_phase_assignment_type(), struct_phase_assignment_capacity);
            }
            return struct_phase_transition_array;
        }

        /** Returns the LLVM representation of one fixed-index phase descriptor group. */
        auto struct_phase_group_type() -> llvm::StructType*
        {
            if (struct_phase_group_struct == nullptr)
            {
                struct_phase_group_struct = llvm::StructType::create(context, {
                    struct_phase_transition_type(),
                    opaque_pointer_type(),
                    opaque_pointer_type(),
                    opaque_pointer_type(),
                }, "qux.struct_phase_group");
            }
            return struct_phase_group_struct;
        }

        /** Returns a deterministic private name suffix for one canonical subobject identity. */
        static auto struct_subobject_symbol_suffix(quxlang::struct_subobject_id const& subobject) -> std::string
        {
            std::string result = subobject.virtual_root.has_value() ? ".virtual." + quxlang::to_string(*subobject.virtual_root) : ".root";
            for (std::size_t const ordinal : subobject.nonvirtual_path)
            {
                result += ".base" + std::to_string(ordinal);
            }
            return result;
        }

        /** Returns a deterministic private name suffix for one runtime phase. */
        static auto struct_phase_symbol_suffix(quxlang::struct_phase_key const& phase) -> std::string
        {
            std::string kind;
            switch (phase.kind)
            {
            case quxlang::struct_phase_kind::steady:
                kind = ".steady";
                break;
            case quxlang::struct_phase_kind::construction:
                kind = ".construction";
                break;
            case quxlang::struct_phase_kind::destruction:
                kind = ".destruction";
                break;
            }
            return kind + ".active." + quxlang::to_string(phase.active_type) + struct_subobject_symbol_suffix(phase.active_subobject);
        }

        /** Embeds an active-type-relative subobject identity in its complete-object context. */
        static auto embed_struct_phase_subobject(quxlang::struct_subobject_id const& active_subobject, quxlang::struct_subobject_id const& relative_subobject) -> quxlang::struct_subobject_id
        {
            if (relative_subobject.virtual_root.has_value())
            {
                return relative_subobject;
            }
            quxlang::struct_subobject_id result = active_subobject;
            result.nonvirtual_path.insert(result.nonvirtual_path.end(), relative_subobject.nonvirtual_path.begin(), relative_subobject.nonvirtual_path.end());
            return result;
        }

        /** Returns one canonical subobject record from backend-ready runtime information. */
        static auto find_struct_runtime_subobject(quxlang::struct_runtime_info const& runtime, quxlang::struct_subobject_id const& identity) -> quxlang::struct_runtime_subobject const&
        {
            std::vector< quxlang::struct_runtime_subobject >::const_iterator const selected = std::ranges::find_if(runtime.subobjects, [&](quxlang::struct_runtime_subobject const& candidate)
            {
                return candidate.id == identity;
            });
            if (selected == runtime.subobjects.end())
            {
                throw quxlang::compiler_bug("Runtime phase names an unavailable subobject in " + quxlang::to_string(runtime.complete_type));
            }
            return *selected;
        }

        /** Emits one ABI-preserving receiver-adjustment thunk for a virtual slot record. */
        auto emit_struct_adjustment_thunk(quxlang::struct_runtime_info const& runtime, quxlang::struct_adjustment_thunk const& thunk, std::string const& phase_suffix = {}) -> llvm::Function*
        {
            std::map< quxlang::type_symbol, llvm::Function* >::const_iterator const target = functions.find(thunk.target_routine);
            std::map< quxlang::type_symbol, callable_abi >::const_iterator const target_abi = function_abis.find(thunk.target_routine);
            if (target == functions.end() || target_abi == function_abis.end())
            {
                throw quxlang::compiler_bug("Runtime virtual target was not declared: " + quxlang::to_string(thunk.target_routine));
            }

            std::string const name = "__qux_struct_thunk." + quxlang::to_string(runtime.complete_type) + phase_suffix + struct_subobject_symbol_suffix(thunk.source_subobject) + ".slot" + std::to_string(thunk.slot_ordinal);
            llvm::Function* const function = llvm::Function::Create(target_abi->second.llvm_type, llvm::GlobalValue::PrivateLinkage, name, module.get());
            apply_calling_convention(function, target_abi->second);
            llvm::BasicBlock* const entry = llvm::BasicBlock::Create(context, "entry", function);
            llvm::IRBuilder<> thunk_builder(entry);

            std::vector< llvm::Value* > arguments;
            arguments.reserve(function->arg_size());
            for (llvm::Argument& argument : function->args())
            {
                arguments.push_back(&argument);
            }
            std::map< std::string, std::size_t >::const_iterator const this_source = target_abi->second.source_named_indices.find("THIS");
            if (this_source == target_abi->second.source_named_indices.end())
            {
                throw quxlang::compiler_bug("Virtual adjustment thunk target has no THIS parameter");
            }
            std::vector< std::size_t >::const_iterator const this_parameter = std::find(target_abi->second.llvm_param_source_indices.begin(), target_abi->second.llvm_param_source_indices.end(), this_source->second);
            if (this_parameter == target_abi->second.llvm_param_source_indices.end())
            {
                throw quxlang::compiler_bug("Virtual adjustment thunk THIS parameter is not passed to LLVM");
            }
            std::size_t const llvm_this_index = static_cast< std::size_t >(std::distance(target_abi->second.llvm_param_source_indices.cbegin(), this_parameter));
            arguments.at(llvm_this_index) = thunk_builder.CreateGEP(i8_type(), arguments.at(llvm_this_index), llvm::ConstantInt::getSigned(i64_type(), thunk.receiver_adjustment));
            llvm::CallInst* const call = thunk_builder.CreateCall(target_abi->second.llvm_type, target->second, arguments);
            apply_calling_convention(call, target_abi->second);
            if (target_abi->second.llvm_type->getReturnType()->isVoidTy())
            {
                thunk_builder.CreateRetVoid();
            }
            else
            {
                thunk_builder.CreateRet(call);
            }
            return function;
        }

        /** Emits immutable cast, dispatch, and fixed-prefix descriptor constants for reached hierarchies. */
        void emit_struct_runtime_descriptors()
        {
            llvm::IntegerType* const ordinal_type = pointer_integer_type();
            llvm::StructType* const cast_record_type = llvm::StructType::get(context, {ordinal_type, i64_type()});
            std::set< quxlang::struct_phase_descriptor_key > descriptor_keys;
            for (std::pair< quxlang::type_symbol const, quxlang::struct_runtime_info > const& runtime_entry : input.struct_runtime_infos)
            {
                quxlang::struct_runtime_info const& complete_runtime = runtime_entry.second;
                for (quxlang::struct_phase_descriptor_group const& group : complete_runtime.descriptor_groups)
                {
                    std::map< quxlang::type_symbol, quxlang::struct_runtime_info >::const_iterator const active_runtime = input.struct_runtime_infos.find(group.phase.active_type);
                    if (active_runtime == input.struct_runtime_infos.end())
                    {
                        throw quxlang::compiler_bug("Runtime phase active type has no backend information: " + quxlang::to_string(group.phase.active_type));
                    }
                    std::size_t self_assignment_count = 0;
                    std::set< std::int64_t > self_offsets;
                    for (quxlang::struct_runtime_subobject const& relative_source : active_runtime->second.subobjects)
                    {
                        if (!relative_source.has_runtime_header)
                        {
                            continue;
                        }
                        quxlang::struct_subobject_id const source_id = embed_struct_phase_subobject(group.phase.active_subobject, relative_source.id);
                        quxlang::struct_runtime_subobject const& source = find_struct_runtime_subobject(complete_runtime, source_id);
                        descriptor_keys.insert(quxlang::struct_phase_descriptor_key{
                            .complete_type = complete_runtime.complete_type,
                            .phase = group.phase,
                            .source_subobject = source_id,
                        });
                        if (self_offsets.insert(source.offset).second)
                        {
                            ++self_assignment_count;
                        }
                    }
                    struct_phase_assignment_capacity = std::max(struct_phase_assignment_capacity, self_assignment_count);
                    for (std::vector< quxlang::struct_phase_transition > const* transitions : {&group.field_transitions, &group.direct_base_transitions, &group.virtual_base_transitions})
                    {
                        for (quxlang::struct_phase_transition const& transition : *transitions)
                        {
                            struct_phase_assignment_capacity = std::max(struct_phase_assignment_capacity, transition.header_assignments.size());
                            for (quxlang::struct_phase_header_assignment const& assignment : transition.header_assignments)
                            {
                                descriptor_keys.insert(assignment.descriptor);
                            }
                        }
                    }
                }
            }

            llvm::StructType* const descriptor_type = struct_runtime_descriptor_type();
            llvm::StructType* const group_type = struct_phase_group_type();
            for (std::pair< quxlang::type_symbol const, quxlang::struct_runtime_info > const& runtime_entry : input.struct_runtime_infos)
            {
                for (quxlang::struct_phase_descriptor_group const& group : runtime_entry.second.descriptor_groups)
                {
                    std::pair< quxlang::type_symbol, quxlang::struct_phase_key > const key{runtime_entry.first, group.phase};
                    std::string const name = "__qux_struct_phase_group." + quxlang::to_string(runtime_entry.first) + struct_phase_symbol_suffix(group.phase);
                    struct_phase_descriptor_groups.emplace(key, new llvm::GlobalVariable(*module, group_type, true, llvm::GlobalValue::PrivateLinkage, nullptr, name));
                }
            }
            for (quxlang::struct_phase_descriptor_key const& key : descriptor_keys)
            {
                std::string const name = "__qux_struct_descriptor." + quxlang::to_string(key.complete_type) + struct_phase_symbol_suffix(key.phase) + struct_subobject_symbol_suffix(key.source_subobject);
                struct_phase_descriptors.emplace(key, new llvm::GlobalVariable(*module, descriptor_type, true, llvm::GlobalValue::PrivateLinkage, nullptr, name));
            }

            for (quxlang::struct_phase_descriptor_key const& key : descriptor_keys)
            {
                quxlang::struct_runtime_info const& complete_runtime = input.struct_runtime_infos.at(key.complete_type);
                quxlang::struct_runtime_info const& active_runtime = input.struct_runtime_infos.at(key.phase.active_type);
                quxlang::struct_runtime_subobject const& source = find_struct_runtime_subobject(complete_runtime, key.source_subobject);
                std::string const phase_suffix = struct_phase_symbol_suffix(key.phase);

                std::vector< llvm::Constant* > cast_records;
                cast_records.reserve(active_runtime.cast_records.size());
                for (quxlang::struct_runtime_cast_record const& cast_record : active_runtime.cast_records)
                {
                    quxlang::struct_subobject_id const target_id = embed_struct_phase_subobject(key.phase.active_subobject, cast_record.target_subobject);
                    quxlang::struct_runtime_subobject const& target = find_struct_runtime_subobject(complete_runtime, target_id);
                    std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const ordinal = input.type_index_ordinals.find(cast_record.target_type);
                    if (ordinal == input.type_index_ordinals.end())
                    {
                        throw quxlang::compiler_bug("Runtime cast target has no linked type ordinal: " + quxlang::to_string(cast_record.target_type));
                    }
                    cast_records.push_back(llvm::ConstantStruct::get(cast_record_type, {
                        llvm::ConstantInt::get(ordinal_type, ordinal->second),
                        llvm::ConstantInt::getSigned(i64_type(), target.offset),
                    }));
                }
                llvm::ArrayType* const cast_array_type = llvm::ArrayType::get(cast_record_type, cast_records.size());
                std::string const descriptor_suffix = quxlang::to_string(key.complete_type) + phase_suffix + struct_subobject_symbol_suffix(key.source_subobject);
                llvm::GlobalVariable* const cast_array = new llvm::GlobalVariable(*module, cast_array_type, true, llvm::GlobalValue::PrivateLinkage, llvm::ConstantArray::get(cast_array_type, cast_records), "__qux_struct_casts." + descriptor_suffix);

                std::size_t slot_count = 0;
                for (quxlang::struct_adjustment_thunk const& thunk : active_runtime.adjustment_thunks)
                {
                    slot_count = std::max(slot_count, thunk.slot_ordinal + 1);
                }
                std::vector< llvm::Constant* > slot_records(slot_count, llvm::ConstantPointerNull::get(opaque_pointer_type()));
                for (quxlang::struct_adjustment_thunk const& active_thunk : active_runtime.adjustment_thunks)
                {
                    quxlang::struct_subobject_id const thunk_source_id = embed_struct_phase_subobject(key.phase.active_subobject, active_thunk.source_subobject);
                    quxlang::struct_runtime_subobject const& thunk_source = find_struct_runtime_subobject(complete_runtime, thunk_source_id);
                    if (thunk_source.offset != source.offset)
                    {
                        continue;
                    }
                    quxlang::struct_subobject_id const thunk_target_id = embed_struct_phase_subobject(key.phase.active_subobject, active_thunk.target_subobject);
                    quxlang::struct_runtime_subobject const& thunk_target = find_struct_runtime_subobject(complete_runtime, thunk_target_id);
                    quxlang::struct_adjustment_thunk concrete_thunk = active_thunk;
                    concrete_thunk.source_subobject = key.source_subobject;
                    concrete_thunk.target_subobject = thunk_target_id;
                    concrete_thunk.receiver_adjustment = thunk_target.offset - thunk_source.offset;
                    llvm::Function* const thunk_function = emit_struct_adjustment_thunk(complete_runtime, concrete_thunk, phase_suffix);
                    if (!slot_records.at(concrete_thunk.slot_ordinal)->isNullValue())
                    {
                        throw quxlang::compiler_bug("Header-sharing virtual subobjects assign different functions to the same slot ordinal");
                    }
                    slot_records.at(concrete_thunk.slot_ordinal) = llvm::ConstantExpr::getBitCast(thunk_function, opaque_pointer_type());
                }
                llvm::ArrayType* const slot_array_type = llvm::ArrayType::get(opaque_pointer_type(), slot_records.size());
                llvm::GlobalVariable* const slot_array = new llvm::GlobalVariable(*module, slot_array_type, true, llvm::GlobalValue::PrivateLinkage, llvm::ConstantArray::get(slot_array_type, slot_records), "__qux_struct_slots." + descriptor_suffix);

                std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const active_ordinal = input.type_index_ordinals.find(key.phase.active_type);
                if (active_ordinal == input.type_index_ordinals.end())
                {
                    throw quxlang::compiler_bug("Runtime phase type has no linked type ordinal: " + quxlang::to_string(key.phase.active_type));
                }
                llvm::GlobalVariable* const current_group = struct_phase_descriptor_groups.at(std::make_pair(key.complete_type, key.phase));
                quxlang::struct_phase_key destruction_phase = key.phase;
                destruction_phase.kind = quxlang::struct_phase_kind::destruction;
                llvm::GlobalVariable* const destruction_group = struct_phase_descriptor_groups.at(std::make_pair(key.complete_type, destruction_phase));
                llvm::Constant* const descriptor_initializer = llvm::ConstantStruct::get(descriptor_type, {
                    llvm::ConstantInt::get(ordinal_type, active_ordinal->second),
                    llvm::ConstantInt::getSigned(i64_type(), -source.offset),
                    llvm::ConstantInt::get(ordinal_type, complete_runtime.allocation_size),
                    llvm::ConstantInt::get(ordinal_type, complete_runtime.allocation_align),
                    llvm::ConstantExpr::getBitCast(cast_array, opaque_pointer_type()),
                    llvm::ConstantInt::get(ordinal_type, cast_records.size()),
                    llvm::ConstantExpr::getBitCast(slot_array, opaque_pointer_type()),
                    llvm::ConstantInt::get(ordinal_type, slot_records.size()),
                    llvm::ConstantExpr::getBitCast(current_group, opaque_pointer_type()),
                    llvm::ConstantExpr::getBitCast(destruction_group, opaque_pointer_type()),
                });
                struct_phase_descriptors.at(key)->setInitializer(descriptor_initializer);
            }

            llvm::StructType* const assignment_type = struct_phase_assignment_type();
            llvm::ArrayType* const transition_type = struct_phase_transition_type();
            llvm::Constant* const null_descriptor = llvm::ConstantPointerNull::get(opaque_pointer_type());
            for (std::pair< quxlang::type_symbol const, quxlang::struct_runtime_info > const& runtime_entry : input.struct_runtime_infos)
            {
                quxlang::struct_runtime_info const& complete_runtime = runtime_entry.second;
                for (quxlang::struct_phase_descriptor_group const& group : complete_runtime.descriptor_groups)
                {
                    quxlang::struct_runtime_info const& active_runtime = input.struct_runtime_infos.at(group.phase.active_type);
                    quxlang::struct_runtime_subobject const& active = find_struct_runtime_subobject(complete_runtime, group.phase.active_subobject);
                    auto transition_constant = [&](std::vector< quxlang::struct_phase_header_assignment > const& assignments) -> llvm::Constant*
                    {
                        std::vector< llvm::Constant* > values;
                        values.reserve(struct_phase_assignment_capacity);
                        for (quxlang::struct_phase_header_assignment const& assignment : assignments)
                        {
                            values.push_back(llvm::ConstantStruct::get(assignment_type, {
                                llvm::ConstantInt::getSigned(i64_type(), assignment.header_offset),
                                llvm::ConstantExpr::getBitCast(struct_phase_descriptors.at(assignment.descriptor), opaque_pointer_type()),
                            }));
                        }
                        while (values.size() < struct_phase_assignment_capacity)
                        {
                            values.push_back(llvm::ConstantStruct::get(assignment_type, {
                                llvm::ConstantInt::get(i64_type(), 0),
                                null_descriptor,
                            }));
                        }
                        return llvm::ConstantArray::get(transition_type, values);
                    };

                    std::vector< quxlang::struct_phase_header_assignment > self_assignments;
                    std::set< std::int64_t > assigned_offsets;
                    for (quxlang::struct_runtime_subobject const& relative_source : active_runtime.subobjects)
                    {
                        if (!relative_source.has_runtime_header)
                        {
                            continue;
                        }
                        quxlang::struct_subobject_id const source_id = embed_struct_phase_subobject(group.phase.active_subobject, relative_source.id);
                        quxlang::struct_runtime_subobject const& source = find_struct_runtime_subobject(complete_runtime, source_id);
                        if (!assigned_offsets.insert(source.offset).second)
                        {
                            continue;
                        }
                        self_assignments.push_back(quxlang::struct_phase_header_assignment{
                            .header_offset = source.offset - active.offset,
                            .descriptor = quxlang::struct_phase_descriptor_key{
                                .complete_type = complete_runtime.complete_type,
                                .phase = group.phase,
                                .source_subobject = source_id,
                            },
                        });
                    }

                    auto transition_array = [&](std::vector< quxlang::struct_phase_transition > const& transitions, std::string const& category) -> llvm::GlobalVariable*
                    {
                        std::vector< llvm::Constant* > values;
                        values.reserve(transitions.size());
                        for (quxlang::struct_phase_transition const& transition : transitions)
                        {
                            values.push_back(transition_constant(transition.header_assignments));
                        }
                        llvm::ArrayType* const array_type = llvm::ArrayType::get(transition_type, values.size());
                        std::string const name = "__qux_struct_phase_transitions." + quxlang::to_string(complete_runtime.complete_type) + struct_phase_symbol_suffix(group.phase) + "." + category;
                        return new llvm::GlobalVariable(*module, array_type, true, llvm::GlobalValue::PrivateLinkage, llvm::ConstantArray::get(array_type, values), name);
                    };

                    llvm::GlobalVariable* const field_transitions = transition_array(group.field_transitions, "fields");
                    llvm::GlobalVariable* const direct_base_transitions = transition_array(group.direct_base_transitions, "direct_bases");
                    llvm::GlobalVariable* const virtual_base_transitions = transition_array(group.virtual_base_transitions, "virtual_bases");
                    llvm::Constant* const group_initializer = llvm::ConstantStruct::get(group_type, {
                        transition_constant(self_assignments),
                        llvm::ConstantExpr::getBitCast(field_transitions, opaque_pointer_type()),
                        llvm::ConstantExpr::getBitCast(direct_base_transitions, opaque_pointer_type()),
                        llvm::ConstantExpr::getBitCast(virtual_base_transitions, opaque_pointer_type()),
                    });
                    struct_phase_descriptor_groups.at(std::make_pair(complete_runtime.complete_type, group.phase))->setInitializer(group_initializer);
                }
            }
        }

        /**
         * Ensures the LLVM target, MC, and asm subsystems needed for object emission are initialized once per process.
         */
        static void initialize_llvm_target_support(quxlang::machine_target_info const& machine)
        {
            switch (machine.cpu_type)
            {
            case quxlang::cpu::x86_32:
            case quxlang::cpu::x86_64: {
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
            case quxlang::cpu::arm_32: {
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
            case quxlang::cpu::arm_64: {
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
            case quxlang::cpu::riscv_64: {
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
            case quxlang::cpu::z_arch: {
                static bool const initialized = []() -> bool
                {
                    ::LLVMInitializeSystemZTargetInfo();
                    ::LLVMInitializeSystemZTarget();
                    ::LLVMInitializeSystemZTargetMC();
                    ::LLVMInitializeSystemZAsmParser();
                    ::LLVMInitializeSystemZAsmPrinter();
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

    public:
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
        static auto create_target_machine(quxlang::llvm_backend::llvm_compilation_target const& compilation_target) -> std::unique_ptr< llvm::TargetMachine >
        {
            quxlang::machine_target_info const& machine = compilation_target.machine;
            initialize_llvm_target_support(machine);

            std::string const triple_text = quxlang::lookup_llvm_triple(machine);
            llvm::Triple triple(triple_text);
            std::string target_error;
            llvm::Target const* target = llvm::TargetRegistry::lookupTarget(triple, target_error);
            if (target == nullptr)
            {
                throw quxlang::semantic_compilation_error("Failed to lookup LLVM target for " + triple_text + ": " + target_error);
            }

            std::unique_ptr< llvm::MCSubtargetInfo > subtarget_info(target->createMCSubtargetInfo(triple, "generic", ""));
            if (!subtarget_info)
            {
                throw quxlang::semantic_compilation_error("Failed to inspect LLVM target settings for " + triple_text);
            }
            if (!subtarget_info->isCPUStringValid(compilation_target.cpu_name))
            {
                throw quxlang::semantic_compilation_error("Unknown LLVM target CPU " + compilation_target.cpu_name + " for " + triple_text);
            }
            if (compilation_target.tune_cpu.has_value() && !subtarget_info->isCPUStringValid(*compilation_target.tune_cpu))
            {
                throw quxlang::semantic_compilation_error("Unknown LLVM tune CPU " + *compilation_target.tune_cpu + " for " + triple_text);
            }

            llvm::ArrayRef< llvm::SubtargetFeatureKV > available_features = subtarget_info->getAllProcessorFeatures();
            std::string_view target_features = compilation_target.target_features;
            std::size_t feature_setting_begin = 0;
            while (feature_setting_begin < target_features.size())
            {
                std::size_t feature_setting_end = target_features.find(',', feature_setting_begin);
                std::string_view feature_setting = target_features.substr(feature_setting_begin, feature_setting_end == std::string::npos ? std::string::npos : feature_setting_end - feature_setting_begin);
                if (feature_setting.size() < 2 || (feature_setting.front() != '+' && feature_setting.front() != '-'))
                {
                    throw quxlang::semantic_compilation_error("Malformed LLVM target feature setting " + std::string(feature_setting) + " for " + triple_text);
                }

                std::string_view feature_name = feature_setting.substr(1);
                bool feature_exists = std::any_of(available_features.begin(), available_features.end(),
                    [feature_name](llvm::SubtargetFeatureKV const& available_feature) -> bool
                    {
                        return feature_name == available_feature.Key;
                    });
                if (!feature_exists)
                {
                    throw quxlang::semantic_compilation_error("Unknown LLVM target feature " + std::string(feature_name) + " for " + triple_text);
                }

                if (feature_setting_end == std::string::npos)
                {
                    break;
                }
                feature_setting_begin = feature_setting_end + 1;
            }
            if (!target_features.empty() && target_features.back() == ',')
            {
                throw quxlang::semantic_compilation_error("Malformed trailing LLVM target feature setting for " + triple_text);
            }

            llvm::TargetOptions options;
            options.ExceptionModel = llvm::ExceptionHandling::DwarfCFI;
            llvm::Reloc::Model reloc_model = llvm::Reloc::Model::Static;
            llvm::CodeModel::Model code_model = llvm::CodeModel::Medium;
            if (machine.pointer_size_bytes() == 8)
            {
                code_model = llvm::CodeModel::Large;
            }

            llvm::CodeGenOptLevel opt_level = llvm_codegen_opt_level(compilation_target.optimization);
            llvm::TargetMachine* raw_machine = target->createTargetMachine(triple, compilation_target.cpu_name, compilation_target.target_features, options, reloc_model, code_model, opt_level);
            if (raw_machine == nullptr)
            {
                throw quxlang::semantic_compilation_error("Failed to create LLVM target machine for " + triple_text);
            }
            return std::unique_ptr< llvm::TargetMachine >(raw_machine);
        }

    public:
        /**
         * Emits one LLVM module to a target object file byte buffer.
         */
        static auto emit_module_object_file(llvm::Module const& source_module, quxlang::llvm_backend::llvm_compilation_target const& target) -> std::vector< std::byte >
        {
            std::unique_ptr< llvm::TargetMachine > object_target_machine = create_target_machine(target);
            std::unique_ptr< llvm::Module > object_module = llvm::CloneModule(source_module);
            object_module->setTargetTriple(llvm::Triple(quxlang::lookup_llvm_triple(target.machine)));
            object_module->setDataLayout(object_target_machine->createDataLayout());
            if (target.machine.os_type == quxlang::os::windows)
            {
                for (llvm::Function& function : *object_module)
                {
                    if (!function.isDeclaration())
                    {
                        function.addFnAttr("probe-stack", quxlang::to_string(quxlang::llvm_backend::runtime_check_stack_symbol()));
                    }
                }
            }

            llvm::SmallVector< char, 0 > object_buffer;
            llvm::raw_svector_ostream object_stream(object_buffer);
            llvm::legacy::PassManager pass_manager;
            if (object_target_machine->addPassesToEmitFile(pass_manager, object_stream, nullptr, llvm::CodeGenFileType::ObjectFile))
            {
                throw quxlang::semantic_compilation_error("Failed to emit LLVM object file for " + source_module.getModuleIdentifier());
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

    private:
        /**
         * Returns true when this aggregate LLVM packet should synthesize a Linux ELF process entrypoint.
         */
        auto should_emit_linux_start() const -> bool
        {
            return input.emit_process_entrypoint && input.whole_module && input.whole_module_output_kind == quxlang::output_kind::executable && input.machine_target.machine.os_type == quxlang::os::linux && input.machine_target.machine.binary_type == quxlang::binary::elf && !input.executable_entry_symbol.has_value();
        }

        /** Returns true when this aggregate LLVM packet should synthesize a macOS Mach-O process entrypoint. */
        auto should_emit_macos_start() const -> bool
        {
            return input.emit_process_entrypoint && input.whole_module && input.whole_module_output_kind == quxlang::output_kind::executable && input.machine_target.machine.os_type == quxlang::os::macos && input.machine_target.machine.binary_type == quxlang::binary::macho && !input.executable_entry_symbol.has_value();
        }

        /** Returns true when a Windows executable needs the compiler-provided process entrypoint. */
        auto should_emit_windows_start() const -> bool
        {
            return input.emit_process_entrypoint && input.whole_module && input.whole_module_output_kind == quxlang::output_kind::executable && input.machine_target.machine.os_type == quxlang::os::windows && input.machine_target.machine.binary_type == quxlang::binary::pe && !input.executable_entry_symbol.has_value();
        }

        /** Returns the main definition selected by a compiler-generated process entrypoint. */
        auto process_entry_main_function() -> llvm::Function*
        {
            llvm::Function* main_function = declared_function(input.target_name);
            if (!input.stepping_support.has_value())
            {
                return main_function;
            }

            std::vector< quxlang::cpu_stepping_configuration > const& steppings = input.stepping_support->steppings;
            if (steppings.empty())
            {
                throw quxlang::semantic_compilation_error("A stepped process entrypoint requires at least stepping 0");
            }
            return main_function_for_stepping(0, steppings.size());
        }

        /** Emits a freestanding Windows process entrypoint that returns the selected Quxlang main result. */
        void emit_windows_start()
        {
            std::string constexpr entry_name = "mainCRTStartup";
            if (module->getFunction(entry_name) != nullptr)
            {
                throw quxlang::semantic_compilation_error("LLVM lowering attempted to redefine " + entry_name + " for " + quxlang::to_string(input.target_name));
            }

            llvm::Function* main_function = process_entry_main_function();
            if (main_function->arg_size() != 0)
            {
                throw quxlang::semantic_compilation_error("Executable entry functanoid must not require arguments: " + quxlang::to_string(input.target_name));
            }

            llvm::Type* const result_type = llvm::Type::getInt32Ty(context);
            llvm::Function* const start_function = llvm::Function::Create(llvm::FunctionType::get(result_type, false), llvm::GlobalValue::ExternalLinkage, entry_name, module.get());
            start_function->setDoesNotThrow();
            llvm::BasicBlock* const entry_block = llvm::BasicBlock::Create(context, "entry", start_function);
            builder.SetInsertPoint(entry_block);

            llvm::Value* exit_code = nullptr;
            if (main_function->getReturnType()->isVoidTy())
            {
                builder.CreateCall(main_function, {});
                exit_code = llvm::ConstantInt::get(result_type, 0);
            }
            else if (main_function->getReturnType()->isIntegerTy())
            {
                llvm::Value* const result = builder.CreateCall(main_function, {});
                unsigned const width = result->getType()->getIntegerBitWidth();
                exit_code = width < 32 ? builder.CreateZExt(result, result_type) : width > 32 ? builder.CreateTrunc(result, result_type) : result;
            }
            else
            {
                throw quxlang::semantic_compilation_error("Executable entry functanoid must return VOID or an integer-like value: " + quxlang::to_string(input.target_name));
            }
            builder.CreateRet(exit_code);
        }

        /**
         * Emits one macOS `_start` routine that calls the selected Qux main function and exits with its return code.
         */
        void emit_macos_start()
        {
            if (module->getFunction("_start") != nullptr)
            {
                throw quxlang::semantic_compilation_error("LLVM lowering attempted to redefine _start for " + quxlang::to_string(input.target_name));
            }

            llvm::Function* main_function = process_entry_main_function();
            if (main_function->arg_size() != 0)
            {
                throw quxlang::semantic_compilation_error("Executable entry functanoid must not require arguments: " + quxlang::to_string(input.target_name));
            }

            llvm::Function* const start_function = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), false), llvm::GlobalValue::ExternalLinkage, "_start", module.get());
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

            emit_macos_exit_syscall(exit_code_value);
            builder.CreateUnreachable();
        }

        /** Emits the architecture-specific macOS process-exit syscall sequence for one exit code value. */
        void emit_macos_exit_syscall(llvm::Value* exit_code_value)
        {
            switch (input.machine_target.machine.cpu_type)
            {
            case quxlang::cpu::x86_64: {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(exit_type, "movq $$0x2000001, %rax\n\tsyscall\n\tud2", "{rdi},~{rax},~{rcx},~{r11},~{memory}", true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            case quxlang::cpu::arm_64: {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(exit_type, "mov x16, #1\n\tsvc #0x80\n\tbrk #1", "{x0},~{x16},~{memory}", true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            default:
                break;
            }

            throw quxlang::semantic_compilation_error("macOS Mach-O _start lowering is not implemented for this CPU kind");
        }

        /**
         * Emits one Linux `_start` routine that calls the selected Quxlang main function and exits with its return code.
         */
        void emit_linux_start()
        {
            if (module->getFunction("_start") != nullptr)
            {
                throw quxlang::semantic_compilation_error("LLVM lowering attempted to redefine _start for " + quxlang::to_string(input.target_name));
            }

            llvm::Function* main_function = process_entry_main_function();
            if (main_function->arg_size() != 0)
            {
                throw quxlang::semantic_compilation_error("Executable entry functanoid must not require arguments: " + quxlang::to_string(input.target_name));
            }

            llvm::Function* const start_function = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), false), llvm::GlobalValue::ExternalLinkage, "_start", module.get());
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
            case quxlang::cpu::x86_64: {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(exit_type, "movq $$60, %rax\n\tsyscall\n\tud2", "{rdi},~{rax},~{rcx},~{r11},~{memory}", true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            case quxlang::cpu::x86_32: {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(exit_type, "movl $$1, %eax\n\tint $$0x80\n\tud2", "{ebx},~{eax},~{memory}", true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            case quxlang::cpu::arm_64: {
                llvm::Type* arg_types[] = {pointer_integer_type()};
                llvm::FunctionType* const exit_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), llvm::ArrayRef< llvm::Type* >(arg_types), false);
                llvm::InlineAsm* const exit_asm = llvm::InlineAsm::get(exit_type, "mov x8, #93\n\tsvc #0\n\tbrk #1", "{x0},~{x8},~{memory}", true);
                builder.CreateCall(exit_asm, {exit_code_value});
                return;
            }
            default:
                break;
            }

            throw quxlang::semantic_compilation_error("Linux ELF _start lowering is not implemented for this CPU kind");
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
         * Returns one already-declared LLVM function by its Quxlang symbol.
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
        auto array_initializer_storage_placement() const -> quxlang::class_placement_info
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

            return quxlang::class_placement_info{.size = record_size, .alignment = record_align};
        }

        /**
         * Produces the integer key used for standard Quxlang float comparisons.
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
            return quxlang::is_ref(type) || quxlang::is_ptr(type) || type.type_is< quxlang::procedure_type >() || type.type_is< quxlang::initguard_lock_type >() || type.type_is< quxlang::address_type >() || interface_runtime_type(type);
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
                if (enum_iter->second.format.bit_width > llvm::IntegerType::MAX_INT_BITS)
                {
                    throw quxlang::lowering_compilation_error("ENUM bit width exceeds the LLVM integer width limit: " + quxlang::to_string(type));
                }
                return static_cast< unsigned >(enum_iter->second.format.bit_width);
            }

            std::map< quxlang::type_symbol, quxlang::flagset_info >::const_iterator flagset_iter = input.flagset_infos.find(type);
            if (flagset_iter != input.flagset_infos.end())
            {
                return static_cast< unsigned >(flagset_iter->second.bits);
            }

            return std::nullopt;
        }

        /** Returns the byte storage width carried by one nominal integer type. */
        auto nominal_integer_storage_bytes(quxlang::type_symbol const& type) const -> std::optional< std::uint64_t >
        {
            std::map< quxlang::type_symbol, quxlang::enum_info >::const_iterator enum_iter = input.enum_infos.find(type);
            if (enum_iter != input.enum_infos.end())
            {
                return enum_iter->second.format.storage_bytes();
            }

            std::map< quxlang::type_symbol, quxlang::flagset_info >::const_iterator flagset_iter = input.flagset_infos.find(type);
            if (flagset_iter != input.flagset_infos.end())
            {
                return flagset_iter->second.storage_bytes;
            }

            return std::nullopt;
        }

        auto nominal_integer_is_signed(quxlang::type_symbol const& type) const -> bool
        {
            std::map< quxlang::type_symbol, quxlang::enum_info >::const_iterator enum_iter = input.enum_infos.find(type);
            return enum_iter != input.enum_infos.end() && enum_iter->second.format.encoding == quxlang::enum_integer_encoding::signed_twos_complement_le;
        }

        /** Returns true when a runtime type contains an artifact-local TYPE_INDEX value. */
        auto contains_type_index(quxlang::type_symbol type) const -> bool
        {
            std::set< quxlang::type_symbol > visited;
            std::vector< quxlang::type_symbol > pending{std::move(type)};
            while (!pending.empty())
            {
                quxlang::type_symbol current = std::move(pending.back());
                pending.pop_back();
                if (!visited.insert(current).second)
                {
                    continue;
                }
                if (current.type_is< quxlang::type_index_type >())
                {
                    return true;
                }
                if (current.type_is< quxlang::nvalue_slot >())
                {
                    pending.push_back(current.get_as< quxlang::nvalue_slot >().target);
                }
                else if (current.type_is< quxlang::dvalue_slot >())
                {
                    pending.push_back(current.get_as< quxlang::dvalue_slot >().target);
                }
                else if (current.type_is< quxlang::ptrref_type >())
                {
                    pending.push_back(current.get_as< quxlang::ptrref_type >().target);
                }
                else if (current.type_is< quxlang::attached_type_reference >())
                {
                    pending.push_back(current.get_as< quxlang::attached_type_reference >().carrying_type);
                }
                else if (current.type_is< quxlang::array_type >())
                {
                    pending.push_back(current.get_as< quxlang::array_type >().element_type);
                }
                else if (current.type_is< quxlang::storage >())
                {
                    for (quxlang::type_symbol const& storable_type : current.get_as< quxlang::storage >().storable_types)
                    {
                        pending.push_back(storable_type);
                    }
                }
                else if (current.type_is< quxlang::procedure_type >())
                {
                    quxlang::sigtype const& signature = current.get_as< quxlang::procedure_type >().signature;
                    for (quxlang::type_symbol const& positional : signature.params.positional)
                    {
                        pending.push_back(positional);
                    }
                    for (std::pair< std::string const, quxlang::type_symbol > const& named : signature.params.named)
                    {
                        pending.push_back(named.second);
                    }
                    if (signature.return_type.has_value())
                    {
                        pending.push_back(*signature.return_type);
                    }
                }
                else if (std::optional< quxlang::type_symbol > atomic_value_type = quxlang::atomic_type_argument(current); atomic_value_type.has_value())
                {
                    pending.push_back(std::move(*atomic_value_type));
                }

                std::map< quxlang::type_symbol, quxlang::struct_layout >::const_iterator const struct_iter = input.struct_layouts.find(current);
                if (struct_iter != input.struct_layouts.end())
                {
                    for (quxlang::struct_field_info const& field : struct_iter->second.fields)
                    {
                        pending.push_back(field.type);
                    }
                }
                std::map< quxlang::type_symbol, quxlang::union_info >::const_iterator const union_iter = input.union_infos.find(current);
                if (union_iter != input.union_infos.end())
                {
                    for (quxlang::union_option_info const& option : union_iter->second.options)
                    {
                        pending.push_back(option.type);
                    }
                }
                std::map< quxlang::type_symbol, quxlang::variant_info >::const_iterator const variant_iter = input.variant_infos.find(current);
                if (variant_iter != input.variant_infos.end())
                {
                    for (quxlang::type_symbol const& alternative : variant_iter->second.alternatives)
                    {
                        pending.push_back(alternative);
                    }
                }
            }
            return false;
        }

        /** Rejects artifact-local type identities at an external callable boundary. */
        void validate_external_callable_type_indices(quxlang::type_symbol const& symbol, quxlang::asm_callable const& callable) const
        {
            for (quxlang::asm_argument_binding const& argument : callable.args)
            {
                if (contains_type_index(argument.type))
                {
                    throw quxlang::semantic_compilation_error("External procedure " + quxlang::to_string(symbol) + " cannot accept TYPE_INDEX values");
                }
            }
            if (callable.return_type.has_value() && contains_type_index(*callable.return_type))
            {
                throw quxlang::semantic_compilation_error("External procedure " + quxlang::to_string(symbol) + " cannot return TYPE_INDEX values");
            }
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

            if (type.type_is< quxlang::bool_type >() || type.type_is< quxlang::byte_type >() || type.type_is< quxlang::int_type >() || type.type_is< quxlang::size_type >() || type.type_is< quxlang::float_type >() || type.type_is< quxlang::procedure_type >() || type.type_is< quxlang::initguard_lock_type >() || type.type_is< quxlang::address_type >() || type.type_is< quxlang::type_index_type >())
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
            if (type.type_is< quxlang::address_type >())
            {
                return opaque_pointer_type();
            }
            if (type.type_is< quxlang::type_index_type >())
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

            std::map< quxlang::type_symbol, quxlang::class_placement_info >::const_iterator placement_iter = input.type_placements.find(type);
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
            std::sort(names.begin(), names.end(),
                      [this](std::string const& lhs, std::string const& rhs)
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
                std::optional< quxlang::type_symbol > runtime_type = quxlang::parameter_runtime_type(quxlang::parameter_instantiation_type(param.second));
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
                std::optional< quxlang::type_symbol > runtime_type = quxlang::parameter_runtime_type(quxlang::parameter_instantiation_type(positional));
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

        /** Applies the program stepping suffix to every function defined by this LLVM unit. */
        void suffix_generated_function_symbols()
        {
            if (!input.suffix_generated_function_symbols)
            {
                return;
            }

            std::string const suffix = "_X" + std::to_string(input.stepping_index);
            std::set< llvm::Function* > renamed_functions;
            for (llvm::Function& function : module->functions())
            {
                if (!function.isDeclaration())
                {
                    if (input.stepping_support.has_value() && (function.getName() == "DETECT_CPU_ARCHINFO" || function.getName() == "PICK_STEPPING"))
                    {
                        continue;
                    }
                    function.setName(function.getName() + suffix);
                    renamed_functions.insert(&function);
                }
            }

            for (std::pair< quxlang::type_symbol const, quxlang::asm_callable > const& asm_function : input.asm_callable_interfaces)
            {
                if (input.extern_procedures.contains(asm_function.first))
                {
                    continue;
                }
                std::map< quxlang::type_symbol, llvm::Function* >::const_iterator const found = functions.find(asm_function.first);
                if (found != functions.end() && renamed_functions.insert(found->second).second)
                {
                    found->second->setName(found->second->getName() + suffix);
                }
            }
        }

        /** Applies the stepping section and target attributes to every definition in this unit. */
        void apply_function_codegen_configuration()
        {
            bool apply_target_attributes = input.machine_target.optimization == quxlang::llvm_backend::optimization_level::release && (input.machine_target.cpu_name != "generic" || !input.machine_target.target_features.empty() || input.machine_target.tune_cpu.has_value());
            if (!input.place_definitions_in_stepping_section && !apply_target_attributes)
            {
                return;
            }

            std::string stepping_section = input.machine_target.machine.binary_type == quxlang::binary::macho ? "__TEXT,__text_s" + std::to_string(input.stepping_index) : ".text_s" + std::to_string(input.stepping_index);
            for (llvm::Function& function : module->functions())
            {
                if (function.isDeclaration())
                {
                    continue;
                }
                if (input.place_definitions_in_stepping_section)
                {
                    function.setSection(stepping_section);
                }
                if (apply_target_attributes)
                {
                    function.addFnAttr("target-cpu", input.machine_target.cpu_name);
                    if (!input.machine_target.target_features.empty())
                    {
                        function.addFnAttr("target-features", input.machine_target.target_features);
                    }
                    if (input.machine_target.tune_cpu.has_value())
                    {
                        function.addFnAttr("tune-cpu", *input.machine_target.tune_cpu);
                    }
                }
            }
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
            std::string text;
            if (procedure.architecture == "ARM32" || procedure.architecture == "ARM64")
            {
                text = quxlang::convert_to_gnu_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name, input.machine_target.machine.binary_type == quxlang::binary::elf);
            }
            else if (procedure.architecture == "Z_ARCH")
            {
                text = quxlang::convert_to_gnu_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name, true);
            }
            else if (procedure.architecture == "X64" || procedure.architecture == "X86")
            {
                text = quxlang::convert_to_x64_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name, input.machine_target.machine.binary_type == quxlang::binary::elf);
            }
            else
            {
                throw quxlang::semantic_compilation_error("Unsupported asm procedure architecture for LLVM lowering: " + procedure.architecture);
            }

            if (input.place_definitions_in_stepping_section)
            {
                std::string section_directive = input.machine_target.machine.binary_type == quxlang::binary::pe ? ".section .text_s" + std::to_string(input.stepping_index) + ",\"xr\"\n" : input.machine_target.machine.binary_type == quxlang::binary::macho ? ".section __TEXT,__text_s" + std::to_string(input.stepping_index) + ",regular,pure_instructions\n" : ".section .text_s" + std::to_string(input.stepping_index) + ",\"ax\",@progbits\n";
                std::size_t text_directive = text.find(".text\n");
                if (text_directive == std::string::npos)
                {
                    throw quxlang::compiler_bug("Converted asm procedure has no text section directive");
                }
                text.replace(text_directive, 6, section_directive);
            }

            if (input.suffix_generated_function_symbols)
            {
                std::set< quxlang::type_symbol > generated_procedures{input.target_name};
                for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& function : input.inlinable_functions)
                {
                    generated_procedures.insert(function.first);
                }
                for (std::pair< quxlang::type_symbol const, quxlang::asm_procedure > const& function : input.asm_functions)
                {
                    generated_procedures.insert(function.first);
                }
                for (std::pair< quxlang::type_symbol const, quxlang::asm_callable > const& function : input.asm_callable_interfaces)
                {
                    if (!input.extern_procedures.contains(function.first))
                    {
                        generated_procedures.insert(function.first);
                    }
                }

                auto is_symbol_body = [](char const ch) -> bool
                {
                    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '$';
                };
                std::string suffix = "_X" + std::to_string(input.stepping_index);
                for (quxlang::type_symbol const& symbol : generated_procedures)
                {
                    std::string original = quxlang::format_asm_symbol_name(symbol_link_name(symbol));
                    std::string replacement = quxlang::format_asm_symbol_name(symbol_link_name(symbol) + suffix);
                    std::size_t position = 0;
                    while ((position = text.find(original, position)) != std::string::npos)
                    {
                        bool starts_at_boundary = position == 0 || !is_symbol_body(text[position - 1]);
                        std::size_t after = position + original.size();
                        bool ends_at_boundary = after == text.size() || !is_symbol_body(text[after]);
                        if (starts_at_boundary && ends_at_boundary)
                        {
                            text.replace(position, original.size(), replacement);
                            position += replacement.size();
                        }
                        else
                        {
                            position += original.size();
                        }
                    }
                }
            }

            if (input.definitions_are_coalescible)
            {
                std::size_t global_directive = text.find(".global ");
                if (global_directive == std::string::npos)
                {
                    throw quxlang::compiler_bug("Converted asm procedure has no global symbol directive");
                }
                if (input.machine_target.machine.binary_type == quxlang::binary::macho)
                {
                    std::size_t global_line_end = text.find('\n', global_directive);
                    if (global_line_end == std::string::npos)
                    {
                        throw quxlang::compiler_bug("Converted asm procedure has an incomplete global symbol directive");
                    }
                    std::string symbol_spelling = text.substr(global_directive + 8, global_line_end - (global_directive + 8));
                    text.insert(global_line_end + 1, ".weak_definition " + symbol_spelling + "\n");
                }
                else
                {
                    text.replace(global_directive, 8, ".weak ");
                }
            }
            return text;
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
            std::size_t positional_index = 0;
            for (quxlang::asm_argument_binding const& argument : callable.args)
            {
                std::optional< std::size_t > argument_positional_index;
                if (!argument.api_name.has_value())
                {
                    argument_positional_index = positional_index;
                    positional_index++;
                }
                ordered.push_back(abi_parameter{
                    .name = argument.api_name,
                    .positional_index = argument_positional_index,
                    .type = argument.type,
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
            if (input.extern_procedures.contains(symbol))
            {
                validate_external_callable_type_indices(symbol, callable);
            }
            callable_abi abi = callable_abi_from_asm_callable(callable);
            std::string const link_name = symbol_link_name(symbol);
            llvm::Function* function = module->getFunction(link_name);
            if (function == nullptr)
            {
                llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::ExternalLinkage;
                if (input.optional_extern_procedures.contains(symbol))
                {
                    linkage = llvm::GlobalValue::ExternalWeakLinkage;
                }
                function = llvm::Function::Create(abi.llvm_type, linkage, link_name, module.get());

                if (input.extern_procedures.contains(symbol))
                {
                    if (input.machine_target.machine.binary_type == quxlang::binary::pe)
                    {
                        function->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
                    }
                    else if (input.machine_target.machine.binary_type == quxlang::binary::elf)
                    {
                        std::map< quxlang::type_symbol, std::string >::const_iterator found_version = input.extern_procedure_versions.find(symbol);
                        if (found_version != input.extern_procedure_versions.end())
                        {
                            module->appendModuleInlineAsm(".symver " + link_name + ", " + link_name + "@" + found_version->second + "\n");
                        }
                    }
                }
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
            if (input.extern_procedures.contains(symbol) && input.machine_target.machine.binary_type == quxlang::binary::pe)
            {
                function->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
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
                .type =
                    quxlang::ptrref_type{
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
         * Builds packed aggregate storage while preserving LLVM pointer relocations at exact semantic byte offsets.
         */
        auto packed_antestatal_storage(std::uint64_t storage_size, std::vector< constant_storage_segment > segments) -> llvm::Constant*
        {
            std::sort(segments.begin(), segments.end(),
                      [](constant_storage_segment const& a, constant_storage_segment const& b)
            {
                return a.offset < b.offset;
            });

            std::vector< llvm::Type* > field_types;
            std::vector< llvm::Constant* > field_values;
            std::uint64_t cursor = 0;
            auto append_padding = [&](std::uint64_t size)
            {
                if (size == 0)
                {
                    return;
                }
                llvm::Constant* const padding = constant_byte_array(std::vector< std::byte >(static_cast< std::size_t >(size), std::byte{0}));
                field_types.push_back(padding->getType());
                field_values.push_back(padding);
            };

            for (constant_storage_segment const& segment : segments)
            {
                if (segment.value == nullptr || segment.offset < cursor || segment.offset + segment.size > storage_size)
                {
                    throw quxlang::semantic_compilation_error("Invalid overlapping antestatal constant storage segment");
                }
                append_padding(segment.offset - cursor);

                llvm::TypeSize const llvm_size = module->getDataLayout().getTypeStoreSize(segment.value->getType());
                if (llvm_size.isScalable() || llvm_size.getFixedValue() > segment.size)
                {
                    throw quxlang::semantic_compilation_error("Antestatal constant relocation exceeds its semantic storage segment");
                }
                field_types.push_back(segment.value->getType());
                field_values.push_back(segment.value);
                append_padding(segment.size - llvm_size.getFixedValue());
                cursor = segment.offset + segment.size;
            }
            append_padding(storage_size - cursor);

            if (field_values.empty())
            {
                return constant_byte_array(std::vector< std::byte >(static_cast< std::size_t >(storage_size), std::byte{0}));
            }
            llvm::StructType* const storage_type = llvm::StructType::get(context, llvm::ArrayRef< llvm::Type* >(field_types), true);
            return llvm::ConstantStruct::get(storage_type, llvm::ArrayRef< llvm::Constant* >(field_values));
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
            llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, initializer->getType(), true, llvm::GlobalValue::PrivateLinkage, initializer, name_stem + "$constbytes$" + std::to_string(helper_counter++));
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

            if (type.type_is< quxlang::type_index_type >() && value.type_is< quxlang::antestatal_type_index >())
            {
                std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const ordinal = input.type_index_ordinals.find(value.get_as< quxlang::antestatal_type_index >().indexed_type);
                if (ordinal == input.type_index_ordinals.end())
                {
                    throw quxlang::compiler_bug("Missing LLVM TYPE_INDEX ordinal for antestatal value");
                }
                std::vector< std::byte > result(storage_size, std::byte{0});
                for (std::size_t index = 0; index < result.size() && index < sizeof(std::uint64_t); ++index)
                {
                    result[index] = static_cast< std::byte >((ordinal->second >> (index * 8)) & 0xffu);
                }
                return result;
            }

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

            if (value.type_is< quxlang::antestatal_fusion >())
            {
                std::map< quxlang::type_symbol, quxlang::fusion_layout >::const_iterator const layout_iter = input.fusion_layouts.find(type);
                if (layout_iter == input.fusion_layouts.end() || !layout_iter->second.is_inline)
                {
                    throw quxlang::semantic_compilation_error("Antestatal fusion constant requires an inline fusion layout: " + quxlang::to_string(type));
                }

                quxlang::fusion_layout const& layout = layout_iter->second;
                quxlang::antestatal_fusion const& fusion_value = value.get_as< quxlang::antestatal_fusion >();
                std::vector< std::byte > result(storage_size, std::byte{0});
                std::uint64_t tag = 0;
                if (fusion_value.state.type_is< quxlang::antestatal_fusion_valueless >())
                {
                    if (!layout.valueless_tag.has_value())
                    {
                        throw quxlang::semantic_compilation_error("Invalid valueless antestatal fusion constant for " + quxlang::to_string(type));
                    }
                    tag = *layout.valueless_tag;
                }
                else
                {
                    quxlang::antestatal_fusion_active const& active = fusion_value.state.get_as< quxlang::antestatal_fusion_active >();
                    tag = active.alternative;
                    quxlang::type_symbol const alternative_type = fusion_alternative_type(type, tag);
                    if (alternative_type.type_is< quxlang::void_type >())
                    {
                        if (active.payload.has_value())
                        {
                            throw quxlang::semantic_compilation_error("VOID antestatal fusion alternative cannot contain a payload");
                        }
                    }
                    else
                    {
                        if (!active.payload.has_value())
                        {
                            throw quxlang::semantic_compilation_error("Non-VOID antestatal fusion alternative requires a payload");
                        }
                        std::vector< std::byte > const payload = materialize_antestatal_bytes(alternative_type, active.payload.value());
                        if (layout.payload_offset + payload.size() > result.size())
                        {
                            throw quxlang::semantic_compilation_error("Antestatal fusion payload exceeds inline storage for " + quxlang::to_string(type));
                        }
                        std::copy(payload.begin(), payload.end(), result.begin() + static_cast< std::ptrdiff_t >(layout.payload_offset));
                    }
                }

                if (!layout.tag_type.type_is< quxlang::int_type >())
                {
                    throw quxlang::compiler_bug("Fusion layout tag type is not an integer");
                }
                std::uint64_t const tag_size = layout.tag_type.get_as< quxlang::int_type >().bits / 8;
                if (layout.tag_offset + tag_size > result.size())
                {
                    throw quxlang::compiler_bug("Fusion tag exceeds declared inline storage");
                }
                for (std::uint64_t i = 0; i < tag_size; ++i)
                {
                    result[static_cast< std::size_t >(layout.tag_offset + i)] = static_cast< std::byte >(static_cast< std::uint8_t >((tag >> (i * 8)) & 0xffu));
                }
                return result;
            }

            if (value.type_is< quxlang::antestatal_struct >())
            {
                std::map< quxlang::type_symbol, quxlang::struct_layout >::const_iterator layout_iter = input.struct_layouts.find(type);
                if (layout_iter == input.struct_layouts.end())
                {
                    throw quxlang::semantic_compilation_error("Missing struct layout for readonly antestatal constant: " + quxlang::to_string(type));
                }

                quxlang::antestatal_struct const& struct_value = value.get_as< quxlang::antestatal_struct >();
                std::vector< std::byte > result(storage_size, std::byte{0});
                for (quxlang::struct_field_info const& field : layout_iter->second.fields)
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

        /** Resolves one non-null nested antestatal object access to an exact byte address and semantic type. */
        auto resolve_antestatal_object(quxlang::antestatal_access const& access, std::optional< quxlang::type_symbol > direct_global_type = std::nullopt) -> resolved_antestatal_object
        {
            if (access.type_is< quxlang::antestatal_access_global >())
            {
                quxlang::type_symbol const& symbol = access.get_as< quxlang::antestatal_access_global >().symbol;
                std::map< quxlang::type_symbol, quxlang::type_symbol >::const_iterator const type_iter = input.object_reference_types.find(symbol);
                quxlang::type_symbol root_type;
                if (type_iter != input.object_reference_types.end())
                {
                    root_type = type_iter->second;
                }
                else if (direct_global_type.has_value())
                {
                    root_type = *direct_global_type;
                }
                else
                {
                    throw quxlang::semantic_compilation_error("Missing readonly global type inventory for nested antestatal access: " + quxlang::to_string(symbol));
                }

                if (root_type.type_is< quxlang::procedure_type >())
                {
                    callable_abi abi = callable_abi_from_signature(root_type.get_as< quxlang::procedure_type >().signature);
                    llvm::Function* callee = get_or_create_external_function(symbol, abi);
                    return resolved_antestatal_object{
                        .pointer = llvm::ConstantExpr::getPointerCast(callee, opaque_pointer_type()),
                        .type = std::move(root_type),
                    };
                }

                llvm::GlobalVariable* global = get_or_create_constant_global(symbol, root_type);
                return resolved_antestatal_object{
                    .pointer = llvm::ConstantExpr::getPointerCast(global, opaque_pointer_type()),
                    .type = std::move(root_type),
                };
            }

            auto offset_pointer = [&](llvm::Constant* base, std::uint64_t offset) -> llvm::Constant*
            {
                llvm::Constant* const byte_offset = llvm::ConstantInt::get(i64_type(), offset);
                return llvm::ConstantExpr::getInBoundsGetElementPtr(i8_type(), base, llvm::ArrayRef< llvm::Constant* >{byte_offset});
            };

            if (access.type_is< quxlang::antestatal_access_field >())
            {
                quxlang::antestatal_access_field const& field_access = access.get_as< quxlang::antestatal_access_field >();
                resolved_antestatal_object object = resolve_antestatal_object(field_access.object);
                std::map< quxlang::type_symbol, quxlang::struct_layout >::const_iterator const layout = input.struct_layouts.find(object.type);
                if (layout == input.struct_layouts.end())
                {
                    throw quxlang::semantic_compilation_error("Missing struct layout for nested antestatal field access: " + quxlang::to_string(object.type));
                }
                for (quxlang::struct_field_info const& field : layout->second.fields)
                {
                    if (field.name == field_access.field_name)
                    {
                        return resolved_antestatal_object{
                            .pointer = offset_pointer(object.pointer, field.offset),
                            .type = field.type,
                        };
                    }
                }
                throw quxlang::semantic_compilation_error("Unknown field in nested antestatal access: " + field_access.field_name);
            }

            if (access.type_is< quxlang::antestatal_access_array_element >())
            {
                quxlang::antestatal_access_array_element const& element = access.get_as< quxlang::antestatal_access_array_element >();
                resolved_antestatal_object array = resolve_antestatal_object(element.array);
                if (!array.type.type_is< quxlang::array_type >())
                {
                    throw quxlang::semantic_compilation_error("Nested antestatal array access does not address an array");
                }
                quxlang::type_symbol const& element_type = array.type.get_as< quxlang::array_type >().element_type;
                std::uint64_t const element_size = slot_size(element_type);
                std::uint64_t const offset = element.index * element_size;
                if (offset + element_size > slot_size(array.type))
                {
                    throw quxlang::semantic_compilation_error("Nested antestatal array access is out of bounds");
                }
                return resolved_antestatal_object{
                    .pointer = offset_pointer(array.pointer, offset),
                    .type = element_type,
                };
            }

            if (access.type_is< quxlang::antestatal_access_fusion_payload >())
            {
                quxlang::antestatal_access_fusion_payload const& payload = access.get_as< quxlang::antestatal_access_fusion_payload >();
                resolved_antestatal_object fusion = resolve_antestatal_object(payload.fusion);
                std::map< quxlang::type_symbol, quxlang::fusion_layout >::const_iterator const layout = input.fusion_layouts.find(fusion.type);
                if (layout == input.fusion_layouts.end() || !layout->second.is_inline)
                {
                    throw quxlang::semantic_compilation_error("Nested antestatal fusion payload access requires an inline fusion layout: " + quxlang::to_string(fusion.type));
                }
                quxlang::type_symbol const alternative_type = fusion_alternative_type(fusion.type, payload.alternative);
                if (alternative_type.type_is< quxlang::void_type >())
                {
                    throw quxlang::semantic_compilation_error("Nested antestatal fusion payload access cannot address VOID");
                }
                return resolved_antestatal_object{
                    .pointer = offset_pointer(fusion.pointer, layout->second.payload_offset),
                    .type = alternative_type,
                };
            }

            throw quxlang::semantic_compilation_error("nullptr is not a nested antestatal object access");
        }

        /** Lowers an antestatal access used by a constant pointer value. */
        auto constant_pointer_from_antestatal_access(quxlang::antestatal_access const& access, quxlang::type_symbol const& target_type) -> llvm::Constant*
        {
            if (access.type_is< quxlang::antestatal_nullptr >())
            {
                return llvm::ConstantPointerNull::get(opaque_pointer_type());
            }
            return resolve_antestatal_object(access, target_type).pointer;
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

            if (type.type_is< quxlang::address_type >())
            {
                if (value.type_is< quxlang::antestatal_ptrref >())
                {
                    return constant_pointer_from_antestatal_access(value.get_as< quxlang::antestatal_ptrref >().target, quxlang::void_type{});
                }
                throw quxlang::semantic_compilation_error("Expected nullptr or pointer readonly antestatal initializer for ADDRESS");
            }

            if (type.type_is< quxlang::type_index_type >())
            {
                if (!value.type_is< quxlang::antestatal_type_index >())
                {
                    throw quxlang::semantic_compilation_error("Expected symbolic readonly antestatal TYPE_INDEX initializer");
                }
                quxlang::type_symbol const& indexed_type = value.get_as< quxlang::antestatal_type_index >().indexed_type;
                std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const ordinal = input.type_index_ordinals.find(indexed_type);
                if (ordinal == input.type_index_ordinals.end())
                {
                    throw quxlang::compiler_bug("Missing LLVM TYPE_INDEX ordinal for " + quxlang::to_string(indexed_type));
                }
                return llvm::ConstantInt::get(pointer_integer_type(), ordinal->second);
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
                llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, interface_constant->getType(), true, llvm::GlobalValue::PrivateLinkage, interface_constant, quxlang::to_string(interface_value.interface_type) + "$iface$const$" + std::to_string(helper_counter++));
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
                llvm::Constant* start_pointer = llvm::ConstantExpr::getInBoundsGetElementPtr(payload->getValueType(), payload, llvm::ArrayRef< llvm::Constant* >{zero, zero});
                llvm::Constant* end_pointer = start_pointer;
                if (!bytes.empty())
                {
                    end_pointer = llvm::ConstantExpr::getInBoundsGetElementPtr(i8_type(), start_pointer, llvm::ArrayRef< llvm::Constant* >{llvm::ConstantInt::get(i64_type(), bytes.size())});
                }

                return llvm::ConstantStruct::get(llvm::cast< llvm::StructType >(value_storage_type(type)), {start_pointer, end_pointer});
            }

            if (type.type_is< quxlang::array_type >() && value.type_is< quxlang::antestatal_array >())
            {
                quxlang::type_symbol const& element_type = type.get_as< quxlang::array_type >().element_type;
                quxlang::antestatal_array const& array_value = value.get_as< quxlang::antestatal_array >();
                std::uint64_t const element_size = slot_size(element_type);
                std::uint64_t const storage_size = slot_size(type);
                std::vector< constant_storage_segment > segments;
                segments.reserve(array_value.elements.size());
                for (std::size_t i = 0; i < array_value.elements.size(); ++i)
                {
                    std::uint64_t const offset = static_cast< std::uint64_t >(i) * element_size;
                    if (offset + element_size > storage_size)
                    {
                        throw quxlang::semantic_compilation_error("Antestatal array initializer exceeds declared storage size");
                    }
                    if (element_size != 0)
                    {
                        segments.push_back(constant_storage_segment{
                            .offset = offset,
                            .size = element_size,
                            .value = create_antestatal_constant_initializer(element_type, array_value.elements[i]),
                        });
                    }
                }
                return packed_antestatal_storage(storage_size, std::move(segments));
            }

            if (value.type_is< quxlang::antestatal_fusion >())
            {
                std::map< quxlang::type_symbol, quxlang::fusion_layout >::const_iterator const layout_iter = input.fusion_layouts.find(type);
                if (layout_iter == input.fusion_layouts.end() || !layout_iter->second.is_inline)
                {
                    throw quxlang::semantic_compilation_error("Antestatal fusion constant requires an inline fusion layout: " + quxlang::to_string(type));
                }
                quxlang::fusion_layout const& layout = layout_iter->second;
                quxlang::antestatal_fusion const& fusion_value = value.get_as< quxlang::antestatal_fusion >();
                std::uint64_t tag = 0;
                std::vector< constant_storage_segment > segments;
                if (fusion_value.state.type_is< quxlang::antestatal_fusion_valueless >())
                {
                    if (!layout.valueless_tag.has_value())
                    {
                        throw quxlang::semantic_compilation_error("Invalid valueless antestatal fusion constant for " + quxlang::to_string(type));
                    }
                    tag = *layout.valueless_tag;
                }
                else
                {
                    quxlang::antestatal_fusion_active const& active = fusion_value.state.get_as< quxlang::antestatal_fusion_active >();
                    tag = active.alternative;
                    quxlang::type_symbol const alternative_type = fusion_alternative_type(type, tag);
                    if (alternative_type.type_is< quxlang::void_type >())
                    {
                        if (active.payload.has_value())
                        {
                            throw quxlang::semantic_compilation_error("VOID antestatal fusion alternative cannot contain a payload");
                        }
                    }
                    else
                    {
                        if (!active.payload.has_value())
                        {
                            throw quxlang::semantic_compilation_error("Non-VOID antestatal fusion alternative requires a payload");
                        }
                        std::uint64_t const payload_size = slot_size(alternative_type);
                        if (payload_size != 0)
                        {
                            segments.push_back(constant_storage_segment{
                                .offset = layout.payload_offset,
                                .size = payload_size,
                                .value = create_antestatal_constant_initializer(alternative_type, active.payload.value()),
                            });
                        }
                    }
                }

                llvm::IntegerType* const tag_type = llvm::cast< llvm::IntegerType >(value_storage_type(layout.tag_type));
                segments.push_back(constant_storage_segment{
                    .offset = layout.tag_offset,
                    .size = slot_size(layout.tag_type),
                    .value = llvm::ConstantInt::get(tag_type, tag),
                });
                return packed_antestatal_storage(layout.placement.size, std::move(segments));
            }

            if (value.type_is< quxlang::antestatal_struct >())
            {
                std::map< quxlang::type_symbol, quxlang::struct_layout >::const_iterator const layout_iter = input.struct_layouts.find(type);
                if (layout_iter == input.struct_layouts.end())
                {
                    throw quxlang::semantic_compilation_error("Missing struct layout for readonly antestatal constant: " + quxlang::to_string(type));
                }
                quxlang::antestatal_struct const& struct_value = value.get_as< quxlang::antestatal_struct >();
                std::vector< constant_storage_segment > segments;
                segments.reserve(layout_iter->second.fields.size());
                for (quxlang::struct_field_info const& field : layout_iter->second.fields)
                {
                    std::map< std::string, quxlang::antestatal_value >::const_iterator const field_value = struct_value.fields.find(field.name);
                    if (field_value == struct_value.fields.end())
                    {
                        throw quxlang::semantic_compilation_error("Missing field '" + field.name + "' in readonly antestatal constant for " + quxlang::to_string(type));
                    }
                    std::uint64_t const field_size = slot_size(field.type);
                    if (field_size != 0)
                    {
                        segments.push_back(constant_storage_segment{
                            .offset = field.offset,
                            .size = field_size,
                            .value = create_antestatal_constant_initializer(field.type, field_value->second),
                        });
                    }
                }
                return packed_antestatal_storage(slot_size(type), std::move(segments));
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
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(*module, initializer->getType(), true, llvm::GlobalValue::PrivateLinkage, initializer, name_stem + "$strconst$" + std::to_string(helper_counter++));
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

        /**
         * Returns the LLVM value for one source-order MODULE(RUNTIME)::PANIC argument.
         */
        auto runtime_panic_argument_value(quxlang::llvm_backend::runtime_panic_call_arguments const& args, abi_parameter const& parameter) -> llvm::Value*
        {
            if (!parameter.name.has_value())
            {
                throw quxlang::semantic_compilation_error("Runtime PANIC argument must be named");
            }

            std::string const& name = *parameter.name;
            if (name == "message")
            {
                llvm::GlobalVariable* const object = create_private_runtime_string_constant(args.message, quxlang::to_string(input.target_name));
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

            throw quxlang::semantic_compilation_error("Unknown Runtime PANIC argument: " + name);
        }

        /**
         * Returns MODULE(RUNTIME)::PANIC call operands in selected ABI order.
         */
        auto runtime_panic_call_arguments(quxlang::llvm_backend::runtime_panic_call_arguments const& args, callable_abi const& abi) -> std::vector< llvm::Value* >
        {
            std::vector< llvm::Value* > values;
            values.reserve(abi.llvm_param_source_indices.size());
            for (std::size_t const source_index : abi.llvm_param_source_indices)
            {
                values.push_back(runtime_panic_argument_value(args, abi.source_ordered.at(source_index)));
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

            llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, storage_type, is_constant, llvm::GlobalValue::ExternalLinkage, nullptr, quxlang::to_string(symbol));
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

        auto should_emit_main_function_array_target() const -> bool
        {
            return input.defines_compiler_builtin_objects && input.whole_module && input.whole_module_output_kind.has_value() && (*input.whole_module_output_kind == quxlang::output_kind::executable || *input.whole_module_output_kind == quxlang::output_kind::unit_test_suite) && (!input.post_detect_functanoid.has_value() || input.target_name != *input.post_detect_functanoid);
        }

        /** Returns whether this unit owns the selected output category's post-detect dispatch table. */
        auto should_emit_post_detect_function_array_target() const -> bool
        {
            return input.defines_compiler_builtin_objects && input.whole_module && input.whole_module_output_kind.has_value() && (*input.whole_module_output_kind == quxlang::output_kind::executable || *input.whole_module_output_kind == quxlang::output_kind::unit_test_suite);
        }

        auto should_emit_unit_test_objects() const -> bool
        {
            return input.unit_test_objects == quxlang::llvm_backend::unit_test_object_emission::definitions;
        }

        /** Returns whether this component must leave aggregate-owned unit-test objects unresolved. */
        auto should_declare_unit_test_objects() const -> bool
        {
            return input.unit_test_objects == quxlang::llvm_backend::unit_test_object_emission::external_declarations;
        }

        /** Returns the configured element count from the exact MAIN_FUNCTION_ARRAY object type. */
        auto main_function_array_count(quxlang::type_symbol const& object_type) const -> std::size_t
        {
            if (!object_type.type_is< quxlang::array_type >())
            {
                throw quxlang::semantic_compilation_error("MAIN_FUNCTION_ARRAY object must have an array type");
            }

            quxlang::array_type const& array = object_type.get_as< quxlang::array_type >();
            if (!array.element_count.type_is< quxlang::expression_numeric_literal >())
            {
                throw quxlang::semantic_compilation_error("MAIN_FUNCTION_ARRAY size must be a numeric literal");
            }

            std::uint64_t const count = quxlang::parsers::str_to_int< std::uint64_t >(array.element_count.get_as< quxlang::expression_numeric_literal >().value);
            if (count == 0 || count > std::numeric_limits< std::size_t >::max())
            {
                throw quxlang::semantic_compilation_error("MAIN_FUNCTION_ARRAY must contain at least one representable stepping");
            }
            if (object_type != quxlang::llvm_backend::main_function_array_object_type(static_cast< std::size_t >(count)))
            {
                throw quxlang::semantic_compilation_error("MAIN_FUNCTION_ARRAY elements must have type PROCEDURE(: I32)");
            }
            if (input.suffix_generated_function_symbols && input.stepping_index >= count)
            {
                throw quxlang::semantic_compilation_error("LLVM main-program stepping index is outside MAIN_FUNCTION_ARRAY");
            }
            return static_cast< std::size_t >(count);
        }

        /** Returns or declares one generated function definition for a configured stepping. */
        auto declared_function_for_stepping(quxlang::type_symbol const& symbol, std::size_t stepping_index, std::size_t stepping_count) -> llvm::Function*
        {
            llvm::Function* current_function = declared_function(symbol);
            if (!input.stepping_support.has_value() && !input.suffix_generated_function_symbols && stepping_count == 1)
            {
                return current_function;
            }
            if (input.suffix_generated_function_symbols && stepping_index == input.stepping_index)
            {
                return current_function;
            }

            std::string function_name = symbol_link_name(symbol) + "_X" + std::to_string(stepping_index);
            if (llvm::Function* existing = module->getFunction(function_name))
            {
                return existing;
            }
            return llvm::Function::Create(current_function->getFunctionType(), llvm::GlobalValue::ExternalLinkage, function_name, module.get());
        }

        /** Returns or declares the main function corresponding to one array index. */
        auto main_function_for_stepping(std::size_t stepping_index, std::size_t stepping_count) -> llvm::Function*
        {
            return declared_function_for_stepping(input.target_name, stepping_index, stepping_count);
        }

        auto get_or_create_main_function_array_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            std::size_t const stepping_count = main_function_array_count(object_type);
            llvm::ArrayType* const storage_type = llvm::ArrayType::get(opaque_pointer_type(), stepping_count);
            llvm::Constant* initializer = nullptr;
            llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::ExternalLinkage;

            if (should_emit_main_function_array_target())
            {
                llvm::Function* const current_main_function = declared_function(input.target_name);
                if (current_main_function->arg_size() != 0 || !current_main_function->getReturnType()->isIntegerTy(32))
                {
                    throw quxlang::semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + quxlang::to_string(input.target_name));
                }

                std::vector< llvm::Constant* > entries;
                entries.reserve(stepping_count);
                for (std::size_t stepping_index = 0; stepping_index < stepping_count; ++stepping_index)
                {
                    entries.push_back(llvm::ConstantExpr::getPointerCast(main_function_for_stepping(stepping_index, stepping_count), opaque_pointer_type()));
                }
                initializer = llvm::ConstantArray::get(storage_type, entries);
                linkage = input.suffix_generated_function_symbols ? llvm::GlobalValue::LinkOnceODRLinkage : llvm::GlobalValue::ExternalLinkage;
            }

            llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, storage_type, true, linkage, initializer, quxlang::to_string(symbol));
            global->setAlignment(llvm::Align(input.machine_target.machine.pointer_align()));
            constant_globals[symbol] = global;
            return global;
        }

        /** Returns the configured element count from the exact POST_DETECT_FUNCTION_ARRAY object type. */
        auto post_detect_function_array_count(quxlang::type_symbol const& object_type) const -> std::size_t
        {
            if (!object_type.type_is< quxlang::array_type >())
            {
                throw quxlang::semantic_compilation_error("POST_DETECT_FUNCTION_ARRAY object must have an array type");
            }
            quxlang::array_type const& array = object_type.get_as< quxlang::array_type >();
            if (!array.element_count.type_is< quxlang::expression_numeric_literal >())
            {
                throw quxlang::semantic_compilation_error("POST_DETECT_FUNCTION_ARRAY size must be a numeric literal");
            }
            std::uint64_t count = quxlang::parsers::str_to_int< std::uint64_t >(array.element_count.get_as< quxlang::expression_numeric_literal >().value);
            if (count == 0 || count > std::numeric_limits< std::size_t >::max())
            {
                throw quxlang::semantic_compilation_error("POST_DETECT_FUNCTION_ARRAY must contain at least one representable stepping");
            }
            if (object_type != quxlang::llvm_backend::post_detect_function_array_object_type(static_cast< std::size_t >(count)))
            {
                throw quxlang::semantic_compilation_error("POST_DETECT_FUNCTION_ARRAY elements must have type PROCEDURE()");
            }
            if (input.suffix_generated_function_symbols && input.stepping_index >= count)
            {
                throw quxlang::semantic_compilation_error("LLVM post-detect stepping index is outside POST_DETECT_FUNCTION_ARRAY");
            }
            return static_cast< std::size_t >(count);
        }

        /** Returns or declares the post-detect function corresponding to one array index. */
        auto post_detect_function_for_stepping(std::size_t stepping_index, std::size_t stepping_count) -> llvm::Function*
        {
            if (!input.post_detect_functanoid.has_value())
            {
                throw quxlang::compiler_bug("A stepped post-detect table requires a concrete POST_DETECT functanoid");
            }
            return declared_function_for_stepping(*input.post_detect_functanoid, stepping_index, stepping_count);
        }

        /** Returns or creates the array mapping stepping IDs to POST_DETECT definitions. */
        auto get_or_create_post_detect_function_array_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            std::size_t stepping_count = post_detect_function_array_count(object_type);
            llvm::ArrayType* storage_type = llvm::ArrayType::get(opaque_pointer_type(), stepping_count);
            llvm::Constant* initializer = nullptr;
            llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::ExternalLinkage;
            if (should_emit_post_detect_function_array_target() && input.post_detect_functanoid.has_value())
            {
                llvm::Function* current_function = declared_function(*input.post_detect_functanoid);
                if (current_function->arg_size() != 0 || !current_function->getReturnType()->isVoidTy())
                {
                    throw quxlang::semantic_compilation_error("MODULE(RUNTIME)::POST_DETECT must have signature FUNCTION()");
                }

                std::vector< llvm::Constant* > entries;
                entries.reserve(stepping_count);
                for (std::size_t stepping_index = 0; stepping_index < stepping_count; ++stepping_index)
                {
                    entries.push_back(llvm::ConstantExpr::getPointerCast(post_detect_function_for_stepping(stepping_index, stepping_count), opaque_pointer_type()));
                }
                initializer = llvm::ConstantArray::get(storage_type, entries);
                linkage = input.suffix_generated_function_symbols ? llvm::GlobalValue::LinkOnceODRLinkage : llvm::GlobalValue::ExternalLinkage;
            }

            llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, storage_type, true, linkage, initializer, quxlang::to_string(symbol));
            global->setAlignment(llvm::Align(input.machine_target.machine.pointer_align()));
            constant_globals[symbol] = global;
            return global;
        }

        /** Emits the compiler-owned CPU detection calls, stepping selector, and stepping-count object. */
        void emit_cpu_stepping_support()
        {
            if (!input.stepping_support.has_value())
            {
                return;
            }

            quxlang::llvm_backend::cpu_stepping_support const& support = *input.stepping_support;
            if (support.steppings.empty())
            {
                throw quxlang::semantic_compilation_error("CPU stepping support requires at least stepping 0");
            }

            std::map< std::string, llvm::GlobalVariable* > enabled_globals;
            for (std::pair< std::string const, quxlang::type_symbol > const& detector : support.attribute_detectors)
            {
                quxlang::type_symbol enabled_symbol = quxlang::builtin_symbol{.name = detector.first + "_ENABLED"};
                enabled_globals.emplace(detector.first, get_or_create_common_zero_initialized_global(enabled_symbol, llvm::Type::getInt1Ty(context)));
            }

            llvm::IntegerType* stepping_type = pointer_integer_type();
            llvm::GlobalVariable* stepping_count = new llvm::GlobalVariable(*module, stepping_type, true, llvm::GlobalValue::ExternalLinkage, llvm::ConstantInt::get(stepping_type, support.steppings.size()), "STEPPING_COUNT");
            stepping_count->setAlignment(llvm::Align(input.machine_target.machine.pointer_align()));
            constant_globals.emplace(quxlang::builtin_symbol{.name = "STEPPING_COUNT"}, stepping_count);

            llvm::FunctionType* detect_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
            llvm::Function* detect_function = llvm::Function::Create(detect_type, llvm::GlobalValue::ExternalLinkage, "DETECT_CPU_ARCHINFO", module.get());
            llvm::BasicBlock* detect_entry = llvm::BasicBlock::Create(context, "entry", detect_function);
            llvm::IRBuilder<> detect_builder(detect_entry);
            for (std::pair< std::string const, quxlang::type_symbol > const& detector : support.attribute_detectors)
            {
                llvm::Function* detector_function = declared_function(detector.second);
                if (detector_function->getFunctionType() != detect_type)
                {
                    throw quxlang::semantic_compilation_error("CPU attribute detector must have signature FUNCTION(): " + quxlang::to_string(detector.second));
                }
                detect_builder.CreateCall(detector_function);
            }
            detect_builder.CreateRetVoid();

            llvm::FunctionType* picker_type = llvm::FunctionType::get(stepping_type, false);
            llvm::Function* picker_function = llvm::Function::Create(picker_type, llvm::GlobalValue::ExternalLinkage, "PICK_STEPPING", module.get());
            llvm::BasicBlock* picker_entry = llvm::BasicBlock::Create(context, "entry", picker_function);
            llvm::IRBuilder<> picker_builder(picker_entry);

            for (std::size_t stepping_index = support.steppings.size(); stepping_index-- > 1;)
            {
                quxlang::cpu_stepping_configuration const& stepping = support.steppings.at(stepping_index);
                llvm::Value* compatible = llvm::ConstantInt::getTrue(context);
                for (std::pair< std::string const, bool > const& attribute : stepping.attributes)
                {
                    llvm::Value* actual = llvm::ConstantInt::getTrue(context);
                    std::map< std::string, quxlang::cpu_attribute_group >::const_iterator const group = quxlang::cpu_attribute_groups.find(attribute.first);
                    if (group == quxlang::cpu_attribute_groups.end())
                    {
                        std::map< std::string, llvm::GlobalVariable* >::const_iterator const enabled = enabled_globals.find(attribute.first);
                        if (enabled == enabled_globals.end())
                        {
                            throw quxlang::compiler_bug("Missing CPU attribute flag for stepping constraint " + attribute.first);
                        }
                        actual = picker_builder.CreateLoad(llvm::Type::getInt1Ty(context), enabled->second);
                    }
                    else
                    {
                        for (std::string const& group_attribute : group->second.attributes)
                        {
                            std::map< std::string, llvm::GlobalVariable* >::const_iterator const enabled = enabled_globals.find(group_attribute);
                            if (enabled == enabled_globals.end())
                            {
                                throw quxlang::compiler_bug("Missing CPU attribute flag for group constraint " + group_attribute);
                            }
                            llvm::Value* const group_attribute_enabled = picker_builder.CreateLoad(llvm::Type::getInt1Ty(context), enabled->second);
                            actual = picker_builder.CreateAnd(actual, group_attribute_enabled);
                        }
                    }
                    if (!attribute.second)
                    {
                        actual = picker_builder.CreateNot(actual);
                    }
                    compatible = picker_builder.CreateAnd(compatible, actual);
                }

                llvm::BasicBlock* selected_block = llvm::BasicBlock::Create(context, "stepping_" + std::to_string(stepping_index), picker_function);
                llvm::BasicBlock* next_block = llvm::BasicBlock::Create(context, "try_lower", picker_function);
                picker_builder.CreateCondBr(compatible, selected_block, next_block);
                picker_builder.SetInsertPoint(selected_block);
                picker_builder.CreateRet(llvm::ConstantInt::get(stepping_type, stepping_index));
                picker_builder.SetInsertPoint(next_block);
            }
            picker_builder.CreateRet(llvm::ConstantInt::get(stepping_type, 0));
        }

        /** Returns whether one symbol is an ordered unit-test procedure in this packet. */
        auto is_unit_test_procedure(quxlang::type_symbol const& symbol) const -> bool
        {
            return std::any_of(input.unit_tests.begin(), input.unit_tests.end(),
                [&](quxlang::llvm_backend::unit_test_entry const& unit_test) -> bool
                {
                    return unit_test.procedure_symbol == symbol;
                });
        }

        /** Declares ordered unit-test procedures not otherwise defined by this packet. */
        void declare_unit_test_procedures()
        {
            llvm::FunctionType* procedure_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
            for (quxlang::llvm_backend::unit_test_entry const& unit_test : input.unit_tests)
            {
                if (functions.contains(unit_test.procedure_symbol))
                {
                    continue;
                }
                llvm::Function* procedure = llvm::Function::Create(procedure_type, llvm::GlobalValue::ExternalLinkage, symbol_link_name(unit_test.procedure_symbol), module.get());
                functions.emplace(unit_test.procedure_symbol, procedure);
            }
        }

        /** Declares the aggregate-owned post-detect dispatch target without importing its body. */
        void declare_post_detect_procedure()
        {
            if (!input.post_detect_functanoid.has_value() || functions.contains(*input.post_detect_functanoid))
            {
                return;
            }
            llvm::FunctionType* procedure_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
            llvm::Function* procedure = llvm::Function::Create(procedure_type, llvm::GlobalValue::ExternalLinkage, symbol_link_name(*input.post_detect_functanoid), module.get());
            functions.emplace(*input.post_detect_functanoid, procedure);
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
            llvm::GlobalVariable* const table = new llvm::GlobalVariable(*module, array_type, true, llvm::GlobalValue::PrivateLinkage, initializer, quxlang::to_string(input.target_name) + "$unit_test_names$" + std::to_string(helper_counter++));
            table->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            table->setAlignment(llvm::Align(slot_alignment(string_constant_type)));

            llvm::Constant* const zero = llvm::ConstantInt::get(i64_type(), 0);
            llvm::Constant* const first = llvm::ConstantExpr::getInBoundsGetElementPtr(array_type, table, llvm::ArrayRef< llvm::Constant* >{zero, zero});
            return llvm::ConstantExpr::getPointerCast(first, opaque_pointer_type());
        }

        auto create_unit_test_proc_array_pointer() -> llvm::Constant*
        {
            if (!should_emit_unit_test_objects() || input.unit_tests.empty())
            {
                return llvm::ConstantPointerNull::get(opaque_pointer_type());
            }

            std::size_t stepping_count = input.stepping_support.has_value() ? input.stepping_support->steppings.size() : 1;
            if (stepping_count == 0)
            {
                throw quxlang::semantic_compilation_error("A unit-test procedure table requires at least stepping 0");
            }

            std::vector< llvm::Constant* > entries;
            entries.reserve(input.unit_tests.size() * stepping_count);
            for (std::size_t stepping_index = 0; stepping_index < stepping_count; ++stepping_index)
            {
                for (quxlang::llvm_backend::unit_test_entry const& unit_test : input.unit_tests)
                {
                    llvm::Function* procedure = declared_function_for_stepping(unit_test.procedure_symbol, stepping_index, stepping_count);
                    entries.push_back(llvm::ConstantExpr::getPointerCast(procedure, opaque_pointer_type()));
                }
            }

            llvm::ArrayType* array_type = llvm::ArrayType::get(opaque_pointer_type(), entries.size());
            llvm::Constant* initializer = llvm::ConstantArray::get(array_type, entries);
            llvm::GlobalVariable* table = new llvm::GlobalVariable(*module, array_type, true, llvm::GlobalValue::PrivateLinkage, initializer, quxlang::to_string(input.target_name) + "$unit_test_proc$" + std::to_string(helper_counter++));
            table->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            table->setAlignment(llvm::Align(input.machine_target.machine.pointer_align()));

            llvm::Constant* zero = llvm::ConstantInt::get(i64_type(), 0);
            llvm::Constant* first = llvm::ConstantExpr::getInBoundsGetElementPtr(array_type, table, llvm::ArrayRef< llvm::Constant* >{zero, zero});
            return llvm::ConstantExpr::getPointerCast(first, opaque_pointer_type());
        }

        auto get_or_create_unit_test_count_object_global(quxlang::type_symbol const& symbol, quxlang::type_symbol const& object_type) -> llvm::GlobalVariable*
        {
            std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator existing = constant_globals.find(symbol);
            if (existing != constant_globals.end())
            {
                return existing->second;
            }

            quxlang::type_symbol expected_type = quxlang::int_type{
                .bits = input.machine_target.machine.pointer_size_bytes() * 8,
                .has_sign = false,
            };
            if (object_type != expected_type)
            {
                throw quxlang::semantic_compilation_error("UNIT_TEST_COUNT object must have the canonical unsigned pointer-width integer type");
            }

            llvm::Type* const storage_type = value_storage_type(object_type);
            llvm::Constant* initializer = nullptr;
            if (!should_declare_unit_test_objects())
            {
                initializer = llvm::ConstantInt::get(llvm::cast< llvm::IntegerType >(storage_type), should_emit_unit_test_objects() ? input.unit_tests.size() : 0);
            }
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(*module, storage_type, true, llvm::GlobalValue::ExternalLinkage, initializer, quxlang::to_string(symbol));
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
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(*module, storage_type, true, llvm::GlobalValue::ExternalLinkage, should_declare_unit_test_objects() ? nullptr : create_unit_test_names_array_pointer(), quxlang::to_string(symbol));
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
            llvm::GlobalVariable* const global = new llvm::GlobalVariable(*module, storage_type, true, llvm::GlobalValue::ExternalLinkage, should_declare_unit_test_objects() ? nullptr : create_unit_test_proc_array_pointer(), quxlang::to_string(symbol));
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

                if (quxlang::llvm_backend::is_main_function_array_symbol(object_reference.first))
                {
                    (void)get_or_create_main_function_array_global(object_reference.first, object_reference.second);
                    continue;
                }

                if (quxlang::llvm_backend::is_post_detect_function_array_symbol(object_reference.first))
                {
                    (void)get_or_create_post_detect_function_array_global(object_reference.first, object_reference.second);
                    continue;
                }

                if (quxlang::llvm_backend::is_stepping_count_symbol(object_reference.first))
                {
                    quxlang::type_symbol expected_type = quxlang::int_type{
                        .bits = input.machine_target.machine.pointer_size_bytes() * 8,
                        .has_sign = false,
                    };
                    if (object_reference.second != expected_type)
                    {
                        throw quxlang::semantic_compilation_error("STEPPING_COUNT object must have the canonical unsigned pointer-width integer type");
                    }
                    if (!constant_globals.contains(object_reference.first))
                    {
                        llvm::GlobalVariable* declaration = new llvm::GlobalVariable(*module, pointer_integer_type(), true, llvm::GlobalValue::ExternalLinkage, nullptr, "STEPPING_COUNT");
                        declaration->setAlignment(llvm::Align(input.machine_target.machine.pointer_align()));
                        constant_globals.emplace(object_reference.first, declaration);
                    }
                    continue;
                }

                if (input.antestatal_constants.contains(object_reference.first))
                {
                    (void)get_or_create_constant_global(object_reference.first, object_reference.second);
                    continue;
                }

                if (global_init_type(object_reference.first) == quxlang::initialization_type::init_compiler_builtin)
                {
                    llvm::GlobalVariable* global = get_or_create_global(object_reference.first, value_storage_type(object_reference.second), false);
                    if (input.defines_compiler_builtin_objects)
                    {
                        global->setInitializer(llvm::Constant::getNullValue(global->getValueType()));
                        global->setLinkage(llvm::GlobalValue::CommonLinkage);
                    }
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
                storage_type = initializer->getType();
                linkage = input.whole_module && input.machine_target.machine.binary_type == quxlang::binary::pe && !input.definitions_are_coalescible ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::LinkOnceODRLinkage;
            }

            llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, storage_type, true, linkage, initializer, quxlang::to_string(symbol));
            global->setAlignment(llvm::Align(slot_alignment(target_type)));
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

            llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, value_storage_type(quxlang::initguard_type{}), false, llvm::GlobalValue::CommonLinkage, llvm::Constant::getNullValue(value_storage_type(quxlang::initguard_type{})), initguard_global_symbol_name(symbol));
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
         * Returns the compilation-root-relative source filename associated with the target routine.
         */
        auto primary_source_filename() const -> std::string
        {
            if (!input.source_index.has_value())
            {
                return "<unknown>";
            }

            quxlang::vmir2::source_index const& source_index = input.source_index->get();
            std::optional< quxlang::source_location > const location = routine_debug_location(input.target_code);
            if (location.has_value())
            {
                std::map< std::uint64_t, quxlang::vmir2::indexed_source_file >::const_iterator const file_iter = source_index.files.find(location->file_id);
                if (file_iter != source_index.files.end())
                {
                    return file_iter->second.path();
                }
            }

            if (!source_index.files.empty())
            {
                return source_index.files.begin()->second.path();
            }
            return "<unknown>";
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
            llvm::DISubprogram* const subprogram = debug_builder->createFunction(file, quxlang::to_string(symbol), quxlang::to_string(symbol), file, line, subroutine_type, scope_line, llvm::DINode::FlagZero, llvm::DISubprogram::SPFlagDefinition);
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
                builder.SetCurrentDebugLocation(llvm::DILocation::get(context, std::max< unsigned >(state.function->getSubprogram()->getLine(), 1), 1, state.function->getSubprogram()));
                return;
            }

            std::map< std::uint64_t, quxlang::vmir2::indexed_source_file >::const_iterator file_iter = input.source_index->get().files.find(location->file_id);
            if (file_iter == input.source_index->get().files.end())
            {
                builder.SetCurrentDebugLocation(llvm::DILocation::get(context, std::max< unsigned >(state.function->getSubprogram()->getLine(), 1), 1, state.function->getSubprogram()));
                return;
            }

            quxlang::vmir2::source_position const position = file_iter->second.position(location->begin_index);
            builder.SetCurrentDebugLocation(llvm::DILocation::get(context, static_cast< unsigned >(position.line), static_cast< unsigned >(position.column), state.function->getSubprogram()));
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

        /** Returns the nominal fusion type addressed by a direct object or reference slot. */
        auto fusion_type(function_codegen_state const& state, quxlang::vmir2::local_index slot) const -> quxlang::type_symbol
        {
            quxlang::type_symbol type = state.routine->local_types.at(local_slot_index(slot)).type;
            if (quxlang::is_ref(type))
            {
                type = quxlang::remove_ref(type);
            }
            if (!input.fusion_layouts.contains(type))
            {
                throw quxlang::semantic_compilation_error("Missing fusion layout for VMIR fusion instruction on " + quxlang::to_string(type));
            }
            return type;
        }

        /** Returns the type of one declaration-order fusion alternative. */
        auto fusion_alternative_type(quxlang::type_symbol const& type, std::uint64_t alternative) const -> quxlang::type_symbol
        {
            std::map< quxlang::type_symbol, quxlang::union_info >::const_iterator const union_iter = input.union_infos.find(type);
            if (union_iter != input.union_infos.end())
            {
                if (alternative >= union_iter->second.options.size())
                {
                    throw quxlang::semantic_compilation_error("Fusion alternative ordinal is out of range for " + quxlang::to_string(type));
                }
                return union_iter->second.options.at(static_cast< std::size_t >(alternative)).type;
            }
            std::map< quxlang::type_symbol, quxlang::variant_info >::const_iterator const variant_iter = input.variant_infos.find(type);
            if (variant_iter != input.variant_infos.end())
            {
                if (alternative >= variant_iter->second.alternatives.size())
                {
                    throw quxlang::semantic_compilation_error("Fusion alternative ordinal is out of range for " + quxlang::to_string(type));
                }
                return variant_iter->second.alternatives.at(static_cast< std::size_t >(alternative));
            }
            throw quxlang::semantic_compilation_error("Missing UNION/VARIANT information for " + quxlang::to_string(type));
        }

        /** Returns the byte-addressable object pointer represented by a direct or reference fusion slot. */
        auto fusion_object_pointer(function_codegen_state& state, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            quxlang::type_symbol const& slot_type = state.routine->local_types.at(local_slot_index(slot)).type;
            return quxlang::is_ref(slot_type) ? load_reference_pointer(state, builder, slot) : value_address(state, slot);
        }

        /** Returns a byte-offset address within one fusion object. */
        auto fusion_field_pointer(llvm::Value* object_pointer, std::uint64_t offset) -> llvm::Value*
        {
            llvm::Value* const bytes = builder.CreateBitCast(object_pointer, opaque_pointer_type());
            return builder.CreateInBoundsGEP(i8_type(), bytes, llvm::ConstantInt::get(i64_type(), offset));
        }

        /** Loads one fusion discriminator using the exact layout-selected integer type. */
        auto load_fusion_tag(llvm::Value* object_pointer, quxlang::fusion_layout const& layout) -> llvm::Value*
        {
            llvm::Type* const tag_type = value_storage_type(layout.tag_type);
            return builder.CreateLoad(tag_type, fusion_field_pointer(object_pointer, layout.tag_offset));
        }

        /** Stores one fusion discriminator after payload state has been established. */
        void store_fusion_tag(llvm::Value* object_pointer, quxlang::fusion_layout const& layout, std::uint64_t tag)
        {
            llvm::IntegerType* const tag_type = llvm::cast< llvm::IntegerType >(value_storage_type(layout.tag_type));
            builder.CreateStore(llvm::ConstantInt::get(tag_type, tag), fusion_field_pointer(object_pointer, layout.tag_offset));
        }

        auto load_slot_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot) -> llvm::Value*
        {
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(slot)).type;
            llvm::Type* storage_type = value_storage_type(type);
            llvm::LoadInst* const load = ir_builder.CreateLoad(storage_type, value_address(state, slot));
            load->setAlignment(llvm::Align(slot_alignment(type)));
            return load;
        }

        void store_slot_value(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot, llvm::Value* value)
        {
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(slot)).type;
            if (quxlang::is_atomic_type(type))
            {
                value = logical_atomic_value_to_storage(ir_builder, type, value);
            }
            llvm::StoreInst* const store = ir_builder.CreateStore(value, value_address(state, slot));
            store->setAlignment(llvm::Align(slot_alignment(type)));
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

        /** Loads the descriptor pointer stored at the start of a polymorphic source subobject. */
        auto load_struct_runtime_descriptor(llvm::Value* source_pointer) -> llvm::Value*
        {
            return builder.CreateLoad(opaque_pointer_type(), source_pointer, "struct.descriptor");
        }

        /** Loads one field from the fixed inheritance descriptor prefix. */
        auto load_struct_runtime_descriptor_field(llvm::Value* descriptor, unsigned field_index, llvm::Type* field_type, std::string const& name) -> llvm::Value*
        {
            llvm::Value* const field_pointer = builder.CreateStructGEP(struct_runtime_descriptor_type(), descriptor, field_index);
            return builder.CreateLoad(field_type, field_pointer, name);
        }

        /** Applies one fixed-capacity descriptor-assignment transition without a runtime hierarchy search. */
        void apply_struct_phase_transition(llvm::Value* transition_pointer, llvm::Value* active_pointer)
        {
            llvm::ArrayType* const transition_type = struct_phase_transition_type();
            llvm::StructType* const assignment_type = struct_phase_assignment_type();
            for (std::size_t assignment_index = 0; assignment_index < struct_phase_assignment_capacity; ++assignment_index)
            {
                llvm::Value* const assignment_pointer = builder.CreateInBoundsGEP(transition_type, transition_pointer, {
                    llvm::ConstantInt::get(i64_type(), 0),
                    llvm::ConstantInt::get(i64_type(), assignment_index),
                });
                llvm::Value* const offset_pointer = builder.CreateStructGEP(assignment_type, assignment_pointer, 0);
                llvm::Value* const descriptor_pointer = builder.CreateStructGEP(assignment_type, assignment_pointer, 1);
                llvm::Value* const offset = builder.CreateLoad(i64_type(), offset_pointer, "struct.phase.header.offset");
                llvm::Value* const selected_descriptor = builder.CreateLoad(opaque_pointer_type(), descriptor_pointer, "struct.phase.descriptor");
                llvm::Value* const header_pointer = builder.CreateGEP(i8_type(), active_pointer, offset);
                llvm::Value* const previous_descriptor = builder.CreateLoad(opaque_pointer_type(), header_pointer);
                llvm::Value* const descriptor_is_null = builder.CreateICmpEQ(selected_descriptor, llvm::ConstantPointerNull::get(opaque_pointer_type()));
                builder.CreateStore(builder.CreateSelect(descriptor_is_null, previous_descriptor, selected_descriptor), header_pointer);
            }
        }

        /** Reinstalls every runtime header represented by one active phase group. */
        void apply_struct_phase_group(llvm::Value* group_pointer, llvm::Value* active_pointer)
        {
            llvm::Value* const transition_pointer = builder.CreateStructGEP(struct_phase_group_type(), group_pointer, 0);
            apply_struct_phase_transition(transition_pointer, active_pointer);
        }

        /** Selects and applies one field or base transition by its compile-time selector and ordinal. */
        void apply_struct_delegate_phase_transition(llvm::Value* group_pointer, llvm::Value* active_pointer, quxlang::vmir2::struct_init_selector const& selector)
        {
            unsigned transitions_field = 0;
            std::size_t ordinal = 0;
            if (quxlang::typeis< quxlang::vmir2::struct_init_field_selector >(selector))
            {
                transitions_field = 1;
                ordinal = quxlang::as< quxlang::vmir2::struct_init_field_selector >(selector).field_ordinal;
            }
            else if (quxlang::typeis< quxlang::vmir2::struct_init_direct_base_selector >(selector))
            {
                transitions_field = 2;
                ordinal = quxlang::as< quxlang::vmir2::struct_init_direct_base_selector >(selector).direct_base_ordinal;
            }
            else
            {
                transitions_field = 3;
                ordinal = quxlang::as< quxlang::vmir2::struct_init_virtual_base_selector >(selector).virtual_base_ordinal;
            }
            llvm::Value* const ordinal_value = llvm::ConstantInt::get(pointer_integer_type(), ordinal);
            llvm::Value* const transitions_pointer_address = builder.CreateStructGEP(struct_phase_group_type(), group_pointer, transitions_field);
            llvm::Value* const transitions_pointer = builder.CreateLoad(opaque_pointer_type(), transitions_pointer_address);
            llvm::Value* const transition_pointer = builder.CreateInBoundsGEP(struct_phase_transition_type(), transitions_pointer, ordinal_value);
            apply_struct_phase_transition(transition_pointer, active_pointer);
        }

        /** Installs one statically known complete-object phase and optionally its owned virtual bases. */
        void install_struct_phase_descriptors(quxlang::type_symbol const& complete_type, llvm::Value* complete_pointer, quxlang::struct_phase_kind phase_kind, bool include_virtual_bases)
        {
            std::map< quxlang::type_symbol, quxlang::struct_runtime_info >::const_iterator const runtime = input.struct_runtime_infos.find(complete_type);
            if (runtime == input.struct_runtime_infos.end())
            {
                return;
            }
            std::set< std::int64_t > assigned_offsets;
            for (quxlang::struct_runtime_subobject const& subobject : runtime->second.subobjects)
            {
                if (!subobject.has_runtime_header || (!include_virtual_bases && subobject.id.virtual_root.has_value()) || !assigned_offsets.insert(subobject.offset).second)
                {
                    continue;
                }
                quxlang::struct_phase_descriptor_key const descriptor_key{
                    .complete_type = complete_type,
                    .phase = quxlang::struct_phase_key{
                        .kind = phase_kind,
                        .active_subobject = {},
                        .active_type = complete_type,
                    },
                    .source_subobject = subobject.id,
                };
                std::map< quxlang::struct_phase_descriptor_key, llvm::GlobalVariable* >::const_iterator const descriptor = struct_phase_descriptors.find(descriptor_key);
                if (descriptor == struct_phase_descriptors.end())
                {
                    throw quxlang::compiler_bug("Missing emitted struct phase descriptor for " + quxlang::to_string(complete_type));
                }
                llvm::Value* const header_pointer = subobject.offset == 0 ? complete_pointer : builder.CreateGEP(i8_type(), complete_pointer, llvm::ConstantInt::getSigned(i64_type(), subobject.offset));
                builder.CreateStore(llvm::ConstantExpr::getBitCast(descriptor->second, opaque_pointer_type()), header_pointer);
            }
        }

        /** Emits a nullable RTTI lookup for one unique target subobject and advances the current LLVM block. */
        auto emit_struct_runtime_cast_lookup(llvm::Value* source_pointer, std::uint64_t target_ordinal, llvm::BasicBlock*& current_block) -> llvm::Value*
        {
            llvm::Function* const function = current_block->getParent();
            llvm::BasicBlock* const search_block = llvm::BasicBlock::Create(context, "struct.cast.search", function);
            llvm::BasicBlock* const loop_block = llvm::BasicBlock::Create(context, "struct.cast.loop", function);
            llvm::BasicBlock* const record_block = llvm::BasicBlock::Create(context, "struct.cast.record", function);
            llvm::BasicBlock* const match_block = llvm::BasicBlock::Create(context, "struct.cast.match", function);
            llvm::BasicBlock* const next_block = llvm::BasicBlock::Create(context, "struct.cast.next", function);
            llvm::BasicBlock* const done_block = llvm::BasicBlock::Create(context, "struct.cast.done", function);
            llvm::BasicBlock* const continue_block = llvm::BasicBlock::Create(context, "struct.cast.continue", function);
            llvm::ConstantPointerNull* const null_pointer = llvm::ConstantPointerNull::get(opaque_pointer_type());
            llvm::Value* const source_is_null = builder.CreateICmpEQ(source_pointer, null_pointer);
            builder.CreateCondBr(source_is_null, continue_block, search_block);
            llvm::BasicBlock* const null_predecessor = current_block;

            builder.SetInsertPoint(search_block);
            llvm::Value* const descriptor = load_struct_runtime_descriptor(source_pointer);
            llvm::Value* const complete_adjustment = load_struct_runtime_descriptor_field(descriptor, 1, i64_type(), "struct.complete.adjustment");
            llvm::Value* const complete_pointer = builder.CreateGEP(i8_type(), source_pointer, complete_adjustment);
            llvm::Value* const records_pointer = load_struct_runtime_descriptor_field(descriptor, 4, opaque_pointer_type(), "struct.cast.records");
            llvm::Value* const record_count = load_struct_runtime_descriptor_field(descriptor, 5, pointer_integer_type(), "struct.cast.count");
            builder.CreateBr(loop_block);

            builder.SetInsertPoint(loop_block);
            llvm::PHINode* const index = builder.CreatePHI(pointer_integer_type(), 2, "struct.cast.index");
            llvm::PHINode* const match_count = builder.CreatePHI(pointer_integer_type(), 2, "struct.cast.matches");
            llvm::PHINode* const selected_offset = builder.CreatePHI(i64_type(), 2, "struct.cast.offset");
            index->addIncoming(llvm::ConstantInt::get(pointer_integer_type(), 0), search_block);
            match_count->addIncoming(llvm::ConstantInt::get(pointer_integer_type(), 0), search_block);
            selected_offset->addIncoming(llvm::ConstantInt::getSigned(i64_type(), 0), search_block);
            builder.CreateCondBr(builder.CreateICmpULT(index, record_count), record_block, done_block);

            builder.SetInsertPoint(record_block);
            llvm::StructType* const cast_record_type = llvm::StructType::get(context, {pointer_integer_type(), i64_type()});
            llvm::Value* const record_pointer = builder.CreateGEP(cast_record_type, records_pointer, index);
            llvm::Value* const ordinal_pointer = builder.CreateStructGEP(cast_record_type, record_pointer, 0);
            llvm::Value* const record_ordinal = builder.CreateLoad(pointer_integer_type(), ordinal_pointer);
            builder.CreateCondBr(builder.CreateICmpEQ(record_ordinal, llvm::ConstantInt::get(pointer_integer_type(), target_ordinal)), match_block, next_block);

            builder.SetInsertPoint(match_block);
            llvm::Value* const offset_pointer = builder.CreateStructGEP(cast_record_type, record_pointer, 1);
            llvm::Value* const matched_offset = builder.CreateLoad(i64_type(), offset_pointer);
            llvm::Value* const incremented_matches = builder.CreateAdd(match_count, llvm::ConstantInt::get(pointer_integer_type(), 1));
            builder.CreateBr(next_block);

            builder.SetInsertPoint(next_block);
            llvm::PHINode* const next_match_count = builder.CreatePHI(pointer_integer_type(), 2);
            llvm::PHINode* const next_selected_offset = builder.CreatePHI(i64_type(), 2);
            next_match_count->addIncoming(match_count, record_block);
            next_match_count->addIncoming(incremented_matches, match_block);
            next_selected_offset->addIncoming(selected_offset, record_block);
            next_selected_offset->addIncoming(matched_offset, match_block);
            llvm::Value* const next_index = builder.CreateAdd(index, llvm::ConstantInt::get(pointer_integer_type(), 1));
            builder.CreateBr(loop_block);
            index->addIncoming(next_index, next_block);
            match_count->addIncoming(next_match_count, next_block);
            selected_offset->addIncoming(next_selected_offset, next_block);

            builder.SetInsertPoint(done_block);
            llvm::Value* const unique_match = builder.CreateICmpEQ(match_count, llvm::ConstantInt::get(pointer_integer_type(), 1));
            llvm::Value* const target_pointer = builder.CreateGEP(i8_type(), complete_pointer, selected_offset);
            llvm::Value* const selected_pointer = builder.CreateSelect(unique_match, target_pointer, null_pointer);
            builder.CreateBr(continue_block);

            builder.SetInsertPoint(continue_block);
            llvm::PHINode* const result = builder.CreatePHI(opaque_pointer_type(), 2, "struct.cast.result");
            result->addIncoming(null_pointer, null_predecessor);
            result->addIncoming(selected_pointer, done_block);
            current_block = continue_block;
            return result;
        }

        auto slot_alignment(quxlang::type_symbol const& type) const -> std::uint64_t
        {
            std::map< quxlang::type_symbol, quxlang::class_placement_info >::const_iterator iter = input.type_placements.find(type);
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
            if (std::optional< std::uint64_t > const storage_bytes = nominal_integer_storage_bytes(type); storage_bytes.has_value())
            {
                return input.machine_target.machine.integer_alignment_for_bits(std::max< std::uint64_t >(*storage_bytes, 1) * 8);
            }
            if (is_pointer_valued_type(type) || is_output_slot_type(type))
            {
                return input.machine_target.machine.pointer_align();
            }
            throw quxlang::semantic_compilation_error("Missing type placement for LLVM lowering alignment: " + quxlang::to_string(type));
        }

        auto slot_size(quxlang::type_symbol const& type) const -> std::uint64_t
        {
            std::map< quxlang::type_symbol, quxlang::class_placement_info >::const_iterator iter = input.type_placements.find(type);
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
            if (std::optional< std::uint64_t > const storage_bytes = nominal_integer_storage_bytes(type); storage_bytes.has_value())
            {
                return std::max< std::uint64_t >(*storage_bytes, 1);
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
            if (!(type.type_is< quxlang::int_type >() || type.type_is< quxlang::bool_type >() || type.type_is< quxlang::byte_type >() || type.type_is< quxlang::size_type >() || nominal_integer_runtime_type(type)))
            {
                throw quxlang::semantic_compilation_error("Expected integer-like slot for LLVM lowering: " + quxlang::to_string(type));
            }
            return load_slot_value(state, ir_builder, slot);
        }

        /**
         * Validates that a fixed-integer binary instruction uses one exact semantic type for both operands and its result.
         */
        auto fixed_integer_binary_type(function_codegen_state const& state, quxlang::vmir2::local_index a, quxlang::vmir2::local_index b, quxlang::vmir2::local_index result,
                                       char const* instruction_name) const -> quxlang::type_symbol const&
        {
            quxlang::type_symbol const& a_type = state.routine->local_types.at(local_slot_index(a)).type;
            quxlang::type_symbol const& b_type = state.routine->local_types.at(local_slot_index(b)).type;
            quxlang::type_symbol const& result_type = state.routine->local_types.at(local_slot_index(result)).type;
            if (a_type != b_type || a_type != result_type)
            {
                throw quxlang::compiler_bug(std::string(instruction_name) + ": type mismatch among operands");
            }
            return a_type;
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
        auto field_address_from_base_pointer(function_codegen_state& state, ir_builder_t& ir_builder, llvm::Value* base_pointer, quxlang::type_symbol base_type, std::string const& field_name, quxlang::type_symbol const& field_type) -> llvm::Value*
        {
            if (quxlang::is_ref(base_type))
            {
                base_type = quxlang::remove_ref(base_type);
            }
            if (quxlang::is_ptr(base_type))
            {
                base_type = quxlang::remove_ptr(base_type);
            }

            std::map< quxlang::type_symbol, quxlang::struct_layout >::const_iterator layout_iter = input.struct_layouts.find(base_type);
            if (layout_iter == input.struct_layouts.end())
            {
                throw quxlang::semantic_compilation_error("Missing struct layout for LLVM lowering: " + quxlang::to_string(base_type));
            }

            for (quxlang::struct_field_info const& field : layout_iter->second.fields)
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
            llvm::Value* start_pointer = ir_builder.CreateInBoundsGEP(payload->getValueType(), payload, {llvm::ConstantInt::get(i64_type(), 0), llvm::ConstantInt::get(i64_type(), 0)});
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
                throw quxlang::semantic_compilation_error("Missing named LLVM call argument '" + *source_param.name + "'" + " while resolving source index " + std::to_string(source_index) + " of ABI parameter type " + quxlang::to_string(source_param.type) + "; available named args: [" + available_named + "]" + "; positional arg count: " + std::to_string(args.positional.size()));
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
         * Returns true when this VMIR slot must not control cleanup for the storage it views.
         */
        auto is_cleanup_alias(quxlang::vmir2::slot_state const& slot_state) const -> bool
        {
            return slot_state.delegate_of.has_value() || slot_state.array_delegate_of_initializer.has_value() || slot_state.destroy_delegate || slot_state.is_projection;
        }

        /** Returns whether a completed struct delegate loses its owner on one control-flow edge. */
        auto struct_delegate_needs_cleanup(quxlang::vmir2::slot_state const& slot_state, quxlang::vmir2::state_map const& target_state) const -> bool
        {
            if (!slot_state.delegate_of.has_value() || !slot_state.struct_delegate_selector.has_value() || slot_state.stage != quxlang::vmir2::slot_stage::full || !slot_state.nontrivial_dtor.has_value())
            {
                return false;
            }
            std::map< quxlang::vmir2::local_index, quxlang::vmir2::slot_state >::const_iterator const owner = target_state.find(*slot_state.delegate_of);
            return owner == target_state.end() || !owner->second.alive();
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
            quxlang::type_symbol dtor_symbol;
            quxlang::vmir2::invocation_args args;
            std::map< quxlang::vmir2::local_index, quxlang::vmir2::slot_state >::const_iterator const slot_state = state.current_state.find(slot);
            if (slot_state != state.current_state.end() && slot_state->second.nontrivial_dtor.has_value())
            {
                dtor_symbol = slot_state->second.nontrivial_dtor->func;
                args = slot_state->second.nontrivial_dtor->args;
            }
            else
            {
                std::map< quxlang::type_symbol, quxlang::type_symbol >::const_iterator const dtor_iter = state.routine->non_trivial_dtors.find(slot_type);
                if (dtor_iter == state.routine->non_trivial_dtors.end())
                {
                    return;
                }
                dtor_symbol = dtor_iter->second;
                args.named["THIS"] = slot;
            }

            llvm::Value* enclosing_group = nullptr;
            llvm::Value* enclosing_pointer = nullptr;
            quxlang::type_symbol const destruction_type = quxlang::remove_ref(slot_type);
            std::map< quxlang::type_symbol, quxlang::struct_runtime_info >::const_iterator const destruction_runtime = input.struct_runtime_infos.find(destruction_type);
            bool const is_polymorphic_destructor = destruction_runtime != input.struct_runtime_infos.end() && destruction_runtime->second.requirements.polymorphism != quxlang::struct_polymorphism_kind::none;
            if (is_polymorphic_destructor && slot_state != state.current_state.end() && slot_state->second.delegate_of.has_value() && slot_state->second.struct_delegate_selector.has_value())
            {
                quxlang::vmir2::local_index const owner = *slot_state->second.delegate_of;
                enclosing_pointer = value_address(state, owner);
                llvm::Value* const enclosing_descriptor = load_struct_runtime_descriptor(enclosing_pointer);
                enclosing_group = load_struct_runtime_descriptor_field(enclosing_descriptor, 8, opaque_pointer_type(), "struct.phase.group");
                apply_struct_delegate_phase_transition(enclosing_group, enclosing_pointer, *slot_state->second.struct_delegate_selector);
            }
            else
            {
                if (is_polymorphic_destructor)
                {
                    llvm::Value* const object_pointer = quxlang::is_ref(slot_type) ? load_reference_pointer(state, ir_builder, slot) : value_address(state, slot);
                    install_struct_phase_descriptors(destruction_type, object_pointer, quxlang::struct_phase_kind::destruction, true);
                }
            }

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

            llvm::Function* callee = get_or_create_external_function(dtor_symbol, abi);
            std::vector< llvm::Value* > arguments;
            if (quxlang::is_ref(slot_type))
            {
                if (abi.llvm_param_source_indices.size() != 1 || !abi.source_ordered.at(abi.llvm_param_source_indices.front()).name.has_value() || *abi.source_ordered.at(abi.llvm_param_source_indices.front()).name != "THIS")
                {
                    throw quxlang::semantic_compilation_error("Referenced object destructor must have exactly one runtime THIS parameter: " + quxlang::to_string(dtor_symbol));
                }
                arguments.push_back(load_reference_pointer(state, ir_builder, slot));
            }
            else
            {
                arguments = ordered_call_arguments(state, ir_builder, abi, args);
            }
            apply_calling_convention(ir_builder.CreateCall(abi.llvm_type, callee, arguments), abi);
            if (enclosing_group != nullptr)
            {
                apply_struct_phase_group(enclosing_group, enclosing_pointer);
            }
        }

        /**
         * Emits an initguard complete or abort runtime call for a live lock slot.
         */
        void emit_initguard_runtime_call(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::local_index slot, bool abort_lock)
        {
            llvm::Value* lock_value = load_slot_value(state, ir_builder, slot);
            quxlang::llvm_backend::runtime_procedure const procedure = abort_lock ? quxlang::llvm_backend::runtime_procedure::initguard_abort : quxlang::llvm_backend::runtime_procedure::initguard_complete;
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
            return slot_state.nontrivial_dtor.has_value() || state.routine->non_trivial_dtors.contains(slot_type);
        }

        /**
         * Emits cleanup calls and storage poisoning for live locals that do not survive into the successor state.
         */
        void emit_transition_cleanup(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::state_map const& current_state, quxlang::vmir2::state_map const& target_state)
        {
            for (quxlang::vmir2::state_map::const_reverse_iterator slot_entry = current_state.crbegin(); slot_entry != current_state.crend(); ++slot_entry)
            {
                bool const alive_in_target = target_state.contains(slot_entry->first) && target_state.at(slot_entry->first).alive();
                if (!alive_in_target && struct_delegate_needs_cleanup(slot_entry->second, target_state))
                {
                    emit_slot_destructor_call(state, ir_builder, slot_entry->first);
                }
            }
            for (std::pair< quxlang::vmir2::local_index const, quxlang::vmir2::slot_state > const& slot_entry : current_state)
            {
                quxlang::vmir2::local_index const slot = slot_entry.first;
                quxlang::vmir2::slot_state const& slot_state = slot_entry.second;
                bool const alive_in_target = target_state.contains(slot) && target_state.at(slot).alive();
                if (slot_state.delegate_of.has_value() && slot_state.struct_delegate_selector.has_value())
                {
                    continue;
                }
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
        void emit_post_instruction_array_initializer_progress(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::state_map const& previous_state, quxlang::vmir2::state_map const& current_state)
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
        void emit_post_instruction_poison_cleanup(function_codegen_state& state, ir_builder_t& ir_builder, quxlang::vmir2::state_map const& previous_state, quxlang::vmir2::state_map const& current_state, quxlang::vmir2::vm_instruction const& instruction)
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
            for (quxlang::vmir2::state_map::const_reverse_iterator slot_entry = current_state.crbegin(); slot_entry != current_state.crend(); ++slot_entry)
            {
                bool const alive_in_target = exit_state.contains(slot_entry->first) && exit_state.at(slot_entry->first).alive();
                if (!alive_in_target && struct_delegate_needs_cleanup(slot_entry->second, exit_state))
                {
                    emit_slot_destructor_call(state, ir_builder, slot_entry->first);
                }
            }
            for (std::pair< quxlang::vmir2::local_index const, quxlang::vmir2::slot_state > const& slot_entry : current_state)
            {
                quxlang::vmir2::local_index const slot = slot_entry.first;
                quxlang::vmir2::slot_state const& slot_state = slot_entry.second;
                bool const alive_in_target = exit_state.contains(slot) && exit_state.at(slot).alive();
                if (slot_state.delegate_of.has_value() && slot_state.struct_delegate_selector.has_value())
                {
                    continue;
                }
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
                if (!alive_in_target && struct_delegate_needs_cleanup(slot_entry.second, target_state))
                {
                    return true;
                }
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
        auto cleanup_edge_target(function_codegen_state& state, llvm::BasicBlock* source_block, quxlang::vmir2::state_map const& current_state, quxlang::vmir2::state_map const& target_state, llvm::BasicBlock* target_block) -> llvm::BasicBlock*
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
        void emit_atomic_rmw(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::local_index target, quxlang::vmir2::local_index value, quxlang::atomic_access_mode access_mode, std::optional< quxlang::vmir2::local_index > old_value, llvm::AtomicRMWInst::BinOp op)
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
                std::uint64_t const storage_bits = storage_llvm_type->isPointerTy() ? input.machine_target.machine.pointer_size_bytes() * 8 : llvm::cast< llvm::IntegerType >(storage_llvm_type)->getBitWidth();
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
            llvm::AtomicCmpXchgInst* cmpxchg = builder.CreateAtomicCmpXchg(pointer, current_storage_load, updated_storage_value, llvm::Align(storage_alignment), llvm_rmw_ordering(access_mode), llvm_rmw_cmpxchg_failure_ordering(access_mode));
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

        auto create_private_interface_constant(quxlang::type_symbol const& interface_type, std::map< quxlang::interface_slot_key, quxlang::type_symbol > const& functions_map, bool is_default_value) -> llvm::Constant*
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
                    if (!is_default_value)
                    {
                        throw quxlang::compiler_bug("Non-default interface value is missing slot " + slot.name + " for " + quxlang::to_string(interface_type));
                    }
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
            rpnx::apply_visitor< void >(instruction,
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
            llvm::GlobalVariable* global = new llvm::GlobalVariable(*module, value->getType(), true, llvm::GlobalValue::PrivateLinkage, value, quxlang::to_string(inst.interface_type) + "$iface$" + std::to_string(helper_counter++));
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
            quxlang::type_symbol declaration = inst.what;
            if (declaration.type_is< quxlang::instanciation_reference >())
            {
                declaration = declaration.get_as< quxlang::instanciation_reference >().temploid.templexoid;
            }
            std::map< std::string, quxlang::vmir2::local_index >::const_iterator const this_argument = inst.args.named.find("THIS");
            std::map< quxlang::vmir2::local_index, quxlang::vmir2::slot_state >::const_iterator delegate_state = state.current_state.end();
            llvm::Value* enclosing_group = nullptr;
            llvm::Value* enclosing_pointer = nullptr;
            bool is_struct_constructor = false;
            bool is_polymorphic_constructor = false;
            quxlang::type_symbol constructor_type;
            if (declaration.type_is< quxlang::submember >())
            {
                quxlang::submember const& member = declaration.get_as< quxlang::submember >();
                is_struct_constructor = member.name == "CONSTRUCTOR" || member.name == "FULLOBJECT_CONSTRUCTOR" || member.name == "SUBOBJECT_CONSTRUCTOR";
                constructor_type = member.of;
                std::map< quxlang::type_symbol, quxlang::struct_runtime_info >::const_iterator const constructor_runtime = input.struct_runtime_infos.find(constructor_type);
                is_polymorphic_constructor = is_struct_constructor && constructor_runtime != input.struct_runtime_infos.end() && constructor_runtime->second.requirements.polymorphism != quxlang::struct_polymorphism_kind::none;
            }
            if (is_polymorphic_constructor && this_argument != inst.args.named.end())
            {
                delegate_state = state.current_state.find(this_argument->second);
                if (delegate_state != state.current_state.end() && delegate_state->second.delegate_of.has_value() && delegate_state->second.struct_delegate_selector.has_value())
                {
                    quxlang::vmir2::local_index const owner = *delegate_state->second.delegate_of;
                    enclosing_pointer = value_address(state, owner);
                    llvm::Value* const enclosing_descriptor = load_struct_runtime_descriptor(enclosing_pointer);
                    enclosing_group = load_struct_runtime_descriptor_field(enclosing_descriptor, 8, opaque_pointer_type(), "struct.phase.group");
                    apply_struct_delegate_phase_transition(enclosing_group, enclosing_pointer, *delegate_state->second.struct_delegate_selector);
                }
                else
                {
                    quxlang::type_symbol const& this_slot_type = state.routine->local_types.at(local_slot_index(this_argument->second)).type;
                    if (!quxlang::is_ref(this_slot_type) && !quxlang::is_ptr(this_slot_type))
                    {
                        install_struct_phase_descriptors(constructor_type, value_address(state, this_argument->second), quxlang::struct_phase_kind::construction, true);
                    }
                }
            }

            callable_abi abi = direct_callee_abi(inst.what, inst, state);
            llvm::Function* callee = get_or_create_external_function(inst.what, abi);
            llvm::CallInst* call = builder.CreateCall(abi.llvm_type, callee, ordered_call_arguments(state, builder, abi, inst.args));
            apply_calling_convention(call, abi);
            if (std::optional< quxlang::vmir2::local_index > return_slot = call_return_slot(abi, inst.args); return_slot.has_value())
            {
                store_slot_value(state, builder, *return_slot, call);
                if (declaration.type_is< quxlang::submember >())
                {
                    quxlang::submember const& member = declaration.get_as< quxlang::submember >();
                    if (member.name == "GET_REFERENCE" && member.of.type_is< quxlang::builtin_symbol >())
                    {
                        std::string const& object_name = member.of.get_as< quxlang::builtin_symbol >().name;
                        constexpr std::string_view enabled_suffix = "_ENABLED";
                        if (object_name.ends_with(enabled_suffix))
                        {
                            std::string const attribute_stem = object_name.substr(0, object_name.size() - enabled_suffix.size());
                            std::map< std::string, bool >::const_iterator const fixed = input.machine_target.fixed_cpu_attribute_values.find(attribute_stem);
                            if (fixed != input.machine_target.fixed_cpu_attribute_values.end())
                            {
                                state.fixed_cpu_attribute_references.emplace(*return_slot, fixed->second);
                            }
                        }
                    }
                }
            }
            if (is_polymorphic_constructor && this_argument != inst.args.named.end())
            {
                if (enclosing_group != nullptr)
                {
                    apply_struct_phase_group(enclosing_group, enclosing_pointer);
                    if (quxlang::typeis< quxlang::vmir2::struct_init_field_selector >(*delegate_state->second.struct_delegate_selector))
                    {
                        install_struct_phase_descriptors(constructor_type, value_address(state, this_argument->second), quxlang::struct_phase_kind::steady, true);
                    }
                }
                else
                {
                    quxlang::type_symbol const& this_slot_type = state.routine->local_types.at(local_slot_index(this_argument->second)).type;
                    if (!quxlang::is_ref(this_slot_type) && !quxlang::is_ptr(this_slot_type))
                    {
                        install_struct_phase_descriptors(constructor_type, value_address(state, this_argument->second), quxlang::struct_phase_kind::steady, true);
                    }
                }
            }
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::invoke_virtual const& instruction)
        {
            (void)current_block;
            std::map< std::string, quxlang::vmir2::local_index >::const_iterator const this_argument = instruction.args.named.find("THIS");
            if (this_argument == instruction.args.named.end())
            {
                throw quxlang::compiler_bug("INVOKE_VIRTUAL has no THIS argument");
            }
            quxlang::type_symbol const& receiver_slot_type = state.routine->local_types.at(local_slot_index(this_argument->second)).type;
            quxlang::type_symbol receiver_type = quxlang::is_ref(receiver_slot_type) ? quxlang::remove_ref(receiver_slot_type) : quxlang::remove_ptr(receiver_slot_type);
            std::map< quxlang::type_symbol, quxlang::struct_runtime_info >::const_iterator const runtime = input.struct_runtime_infos.find(receiver_type);
            if (runtime == input.struct_runtime_infos.end())
            {
                throw quxlang::lowering_compilation_error("INVOKE_VIRTUAL is missing runtime information for " + quxlang::to_string(receiver_type));
            }
            std::vector< quxlang::struct_adjustment_thunk >::const_iterator const selected_slot = std::ranges::find_if(runtime->second.adjustment_thunks, [&](quxlang::struct_adjustment_thunk const& thunk)
            {
                return thunk.slot == instruction.slot;
            });
            if (selected_slot == runtime->second.adjustment_thunks.end())
            {
                throw quxlang::compiler_bug("INVOKE_VIRTUAL slot is absent from the receiver runtime information");
            }

            llvm::Value* const receiver_pointer = quxlang::is_ref(receiver_slot_type) ? load_reference_pointer(state, builder, this_argument->second) : load_slot_value(state, builder, this_argument->second);
            llvm::Value* const descriptor = load_struct_runtime_descriptor(receiver_pointer);
            if (instruction.slot.signature.name == "DESTRUCTOR")
            {
                llvm::Value* const complete_adjustment = load_struct_runtime_descriptor_field(descriptor, 1, i64_type(), "struct.complete.adjustment");
                llvm::Value* const complete_pointer = builder.CreateGEP(i8_type(), receiver_pointer, complete_adjustment);
                llvm::Value* const destruction_group = load_struct_runtime_descriptor_field(descriptor, 9, opaque_pointer_type(), "struct.destruction.group");
                apply_struct_phase_group(destruction_group, complete_pointer);
            }
            llvm::Value* const slots_pointer = load_struct_runtime_descriptor_field(descriptor, 6, opaque_pointer_type(), "struct.virtual.slots");
            llvm::Value* const function_pointer_address = builder.CreateGEP(opaque_pointer_type(), slots_pointer, llvm::ConstantInt::get(pointer_integer_type(), selected_slot->slot_ordinal));
            llvm::Value* const function_pointer = builder.CreateLoad(opaque_pointer_type(), function_pointer_address);

            quxlang::vmir2::invoke abi_source{.args = instruction.args};
            callable_abi abi = callable_abi_from_invoke(abi_source, state);
            std::vector< llvm::Value* > arguments = ordered_call_arguments(state, builder, abi, instruction.args);
            std::map< std::string, std::size_t >::const_iterator const this_source_index = abi.source_named_indices.find("THIS");
            if (this_source_index == abi.source_named_indices.end())
            {
                throw quxlang::compiler_bug("INVOKE_VIRTUAL ABI has no THIS parameter");
            }
            std::vector< std::size_t >::const_iterator const this_llvm_index = std::find(abi.llvm_param_source_indices.begin(), abi.llvm_param_source_indices.end(), this_source_index->second);
            if (this_llvm_index == abi.llvm_param_source_indices.end())
            {
                throw quxlang::compiler_bug("INVOKE_VIRTUAL THIS parameter is not passed to LLVM");
            }
            llvm::CallInst* const call = builder.CreateCall(abi.llvm_type, function_pointer, arguments);
            apply_calling_convention(call, abi);
            if (std::optional< quxlang::vmir2::local_index > const return_slot = call_return_slot(abi, instruction.args); return_slot.has_value())
            {
                store_slot_value(state, builder, *return_slot, call);
            }
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
            if (is_procedure_reference)
            {
                callee_value = load_reference_pointer(state, builder, inst.what_index);
            }
            else if (is_reference_callable)
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
            quxlang::type_symbol const& source_type = state.routine->local_types.at(local_slot_index(inst.source_index)).type;
            quxlang::type_symbol const& target_type = state.routine->local_types.at(local_slot_index(inst.target_index)).type;
            llvm::Value* pointer_value = quxlang::is_ref(source_type) ? load_reference_pointer(state, builder, inst.source_index) : load_slot_value(state, builder, inst.source_index);
            if (quxlang::is_ref(target_type))
            {
                store_reference_pointer(state, builder, inst.target_index, pointer_value);
            }
            else
            {
                store_slot_value(state, builder, inst.target_index, pointer_value);
            }
            std::map< quxlang::vmir2::local_index, bool >::const_iterator const fixed_cpu_attribute = state.fixed_cpu_attribute_references.find(inst.source_index);
            if (fixed_cpu_attribute != state.fixed_cpu_attribute_references.end())
            {
                state.fixed_cpu_attribute_references.emplace(inst.target_index, fixed_cpu_attribute->second);
            }
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::inheritance_cast const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& source_slot_type = state.routine->local_types.at(local_slot_index(instruction.source)).type;
            quxlang::type_symbol const& result_slot_type = state.routine->local_types.at(local_slot_index(instruction.result)).type;
            if ((!quxlang::is_ref(source_slot_type) && !quxlang::is_ptr(source_slot_type)) || (!quxlang::is_ref(result_slot_type) && !quxlang::is_ptr(result_slot_type)))
            {
                throw quxlang::semantic_compilation_error("INHERITANCE_CAST requires pointer or reference slots");
            }

            quxlang::type_symbol current_type = quxlang::is_ref(source_slot_type) ? quxlang::remove_ref(source_slot_type) : quxlang::remove_ptr(source_slot_type);
            std::int64_t adjustment = 0;
            for (quxlang::struct_subobject_path_step const& step : instruction.path.steps)
            {
                if (step.kind == quxlang::inheritance_kind::virtual_)
                {
                    break;
                }
                std::map< quxlang::type_symbol, quxlang::struct_layout >::const_iterator const layout = input.struct_layouts.find(current_type);
                if (layout == input.struct_layouts.end())
                {
                    throw quxlang::compiler_bug("INHERITANCE_CAST is missing a layout for " + quxlang::to_string(current_type));
                }
                std::vector< quxlang::struct_base_layout_info >::const_iterator const base = std::ranges::find_if(layout->second.direct_bases, [&](quxlang::struct_base_layout_info const& candidate)
                {
                    return candidate.declaration_ordinal == step.direct_base_ordinal && candidate.type == step.base_type;
                });
                if (base == layout->second.direct_bases.end() || base->kind != quxlang::inheritance_kind::nonvirtual)
                {
                    throw quxlang::compiler_bug("INHERITANCE_CAST path does not match the source struct layout");
                }
                adjustment += base->offset;
                current_type = step.base_type;
            }

            llvm::Value* source_pointer = quxlang::is_ref(source_slot_type) ? load_reference_pointer(state, builder, instruction.source) : load_slot_value(state, builder, instruction.source);
            std::vector< quxlang::struct_subobject_path_step >::const_iterator const virtual_step = std::find_if(instruction.path.steps.begin(), instruction.path.steps.end(), [](quxlang::struct_subobject_path_step const& step)
            {
                return step.kind == quxlang::inheritance_kind::virtual_;
            });
            llvm::Value* adjusted_pointer = source_pointer;
            if (virtual_step != instruction.path.steps.end())
            {
                std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const target_ordinal = input.type_index_ordinals.find(virtual_step->base_type);
                if (target_ordinal == input.type_index_ordinals.end())
                {
                    throw quxlang::compiler_bug("Virtual-base INHERITANCE_CAST target has no linked type ordinal");
                }
                llvm::Value* const prefix_pointer = adjustment == 0 ? source_pointer : builder.CreateGEP(i8_type(), source_pointer, llvm::ConstantInt::getSigned(i64_type(), adjustment));
                adjusted_pointer = emit_struct_runtime_cast_lookup(prefix_pointer, target_ordinal->second, current_block);
                current_type = virtual_step->base_type;
                std::int64_t suffix_adjustment = 0;
                for (std::vector< quxlang::struct_subobject_path_step >::const_iterator step = std::next(virtual_step); step != instruction.path.steps.end(); ++step)
                {
                    if (step->kind == quxlang::inheritance_kind::virtual_)
                    {
                        throw quxlang::compiler_bug("INHERITANCE_CAST path contains more than one canonical virtual edge");
                    }
                    quxlang::struct_layout const& layout = input.struct_layouts.at(current_type);
                    std::vector< quxlang::struct_base_layout_info >::const_iterator const base = std::ranges::find_if(layout.direct_bases, [&](quxlang::struct_base_layout_info const& candidate)
                    {
                        return candidate.declaration_ordinal == step->direct_base_ordinal && candidate.type == step->base_type;
                    });
                    QUXLANG_COMPILER_BUG_IF(base == layout.direct_bases.end(), "Virtual-base INHERITANCE_CAST suffix does not match layout");
                    suffix_adjustment += base->offset;
                    current_type = step->base_type;
                }
                if (suffix_adjustment != 0)
                {
                    llvm::Value* const shifted_pointer = builder.CreateGEP(i8_type(), adjusted_pointer, llvm::ConstantInt::getSigned(i64_type(), suffix_adjustment));
                    adjusted_pointer = builder.CreateSelect(builder.CreateICmpEQ(adjusted_pointer, llvm::ConstantPointerNull::get(opaque_pointer_type())), llvm::ConstantPointerNull::get(opaque_pointer_type()), shifted_pointer);
                }
            }
            else if (adjustment != 0)
            {
                adjusted_pointer = builder.CreateGEP(i8_type(), source_pointer, llvm::ConstantInt::getSigned(i64_type(), adjustment));
                if (quxlang::is_ptr(source_slot_type))
                {
                    llvm::Value* const is_null = builder.CreateICmpEQ(source_pointer, llvm::ConstantPointerNull::get(opaque_pointer_type()));
                    adjusted_pointer = builder.CreateSelect(is_null, llvm::ConstantPointerNull::get(opaque_pointer_type()), adjusted_pointer);
                }
            }
            if (quxlang::is_ref(result_slot_type))
            {
                store_reference_pointer(state, builder, instruction.result, adjusted_pointer);
            }
            else
            {
                store_slot_value(state, builder, instruction.result, adjusted_pointer);
            }
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::struct_dynamic_cast const& instruction)
        {
            std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const target_ordinal = input.type_index_ordinals.find(instruction.target_type);
            if (target_ordinal == input.type_index_ordinals.end())
            {
                throw quxlang::compiler_bug("STRUCT_DYNAMIC_CAST target has no linked type ordinal");
            }
            llvm::Value* const source_pointer = load_slot_value(state, builder, instruction.source);
            llvm::Value* const result_pointer = emit_struct_runtime_cast_lookup(source_pointer, target_ordinal->second, current_block);
            store_slot_value(state, builder, instruction.result, result_pointer);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::struct_type_is const& instruction)
        {
            std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const target_ordinal = input.type_index_ordinals.find(instruction.target_type);
            if (target_ordinal == input.type_index_ordinals.end())
            {
                throw quxlang::compiler_bug("STRUCT_TYPE_IS target has no linked type ordinal");
            }
            llvm::Value* const source_pointer = load_slot_value(state, builder, instruction.source);
            llvm::Value* const is_nonnull = builder.CreateICmpNE(source_pointer, llvm::ConstantPointerNull::get(opaque_pointer_type()));
            llvm::Function* const function = current_block->getParent();
            llvm::BasicBlock* const compare_block = llvm::BasicBlock::Create(context, "struct.type_is.compare", function);
            llvm::BasicBlock* const continue_block = llvm::BasicBlock::Create(context, "struct.type_is.continue", function);
            llvm::BasicBlock* const null_predecessor = current_block;
            builder.CreateCondBr(is_nonnull, compare_block, continue_block);
            builder.SetInsertPoint(compare_block);
            llvm::Value* const descriptor = load_struct_runtime_descriptor(source_pointer);
            llvm::Value* const dynamic_ordinal = load_struct_runtime_descriptor_field(descriptor, 0, pointer_integer_type(), "struct.dynamic.ordinal");
            llvm::Value* const type_matches = builder.CreateICmpEQ(dynamic_ordinal, llvm::ConstantInt::get(pointer_integer_type(), target_ordinal->second));
            builder.CreateBr(continue_block);
            builder.SetInsertPoint(continue_block);
            llvm::PHINode* const result = builder.CreatePHI(llvm::Type::getInt1Ty(context), 2, "struct.type_is.result");
            result->addIncoming(llvm::ConstantInt::getFalse(context), null_predecessor);
            result->addIncoming(type_matches, compare_block);
            store_boolean(state, builder, instruction.result, result);
            current_block = continue_block;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::struct_alloc_info const& instruction)
        {
            (void)current_block;
            llvm::Value* const source_pointer = load_slot_value(state, builder, instruction.source);
            llvm::Value* const descriptor = load_struct_runtime_descriptor(source_pointer);
            llvm::Value* const complete_adjustment = load_struct_runtime_descriptor_field(descriptor, 1, i64_type(), "struct.complete.adjustment");
            llvm::Value* const complete_pointer = builder.CreateGEP(i8_type(), source_pointer, complete_adjustment);
            llvm::Value* const allocation_size = load_struct_runtime_descriptor_field(descriptor, 2, pointer_integer_type(), "struct.allocation.size");
            llvm::Value* const allocation_align = load_struct_runtime_descriptor_field(descriptor, 3, pointer_integer_type(), "struct.allocation.align");
            store_slot_value(state, builder, instruction.storage_pointer, complete_pointer);
            store_slot_value(state, builder, instruction.size, allocation_size);
            store_slot_value(state, builder, instruction.align, allocation_align);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::address_launder const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::address_launder const& inst = instruction;
            quxlang::type_symbol const& source_type = state.routine->local_types.at(local_slot_index(inst.source_index)).type;
            quxlang::type_symbol const& target_type = state.routine->local_types.at(local_slot_index(inst.target_index)).type;
            llvm::Value* pointer_value = quxlang::is_ref(source_type) ? load_reference_pointer(state, builder, inst.source_index) : load_slot_value(state, builder, inst.source_index);
            if (quxlang::is_ref(target_type))
            {
                store_reference_pointer(state, builder, inst.target_index, pointer_value);
            }
            else
            {
                store_slot_value(state, builder, inst.target_index, pointer_value);
            }
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::cast_constant const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::cast_constant const& inst = instruction;
            // Source is a CONST& readonly_constant; load the referenced {__start, __end} span and
            // store it into the destination readonly_constant value (same layout, different kind).
            quxlang::type_symbol target_type = state.routine->local_types.at(local_slot_index(inst.target_index)).type;
            llvm::Value* pointer_value = load_reference_pointer(state, builder, inst.source_index);
            llvm::Value* loaded = builder.CreateLoad(value_storage_type(target_type), pointer_value);
            store_slot_value(state, builder, inst.target_index, loaded);
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
            // Check that the literal fits the target int type before constructing APInt
            // (APInt silently truncates on overflow)
            int_type check_type;
            if (typeis< int_type >(target_type))
            {
                check_type = as< int_type >(target_type);
            }
            else if (typeis< byte_type >(target_type))
            {
                check_type = int_type{.bits = 8, .has_sign = false};
            }
            else
            {
                check_type = int_type{.bits = static_cast<std::size_t>(integer_type->getBitWidth()), .has_sign = false};
            }
            std::string full_value = (is_negative ? "-" : "") + digits;
            if (!literal_fits_int(full_value, check_type))
            {
                throw std::overflow_error("Integer literal " + inst.value + " does not fit in target type");
            }
            llvm::APInt value(integer_type->getBitWidth(), digits, 10);
            if (is_negative)
            {
                value = -value;
            }
            store_slot_value(state, builder, inst.target, llvm::ConstantInt::get(integer_type, value));
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_type_index const& instruction)
        {
            (void)current_block;
            std::map< quxlang::type_symbol, std::uint64_t >::const_iterator const ordinal = input.type_index_ordinals.find(instruction.indexed_type);
            if (ordinal == input.type_index_ordinals.end())
            {
                throw quxlang::compiler_bug("Missing LLVM TYPE_INDEX ordinal for " + quxlang::to_string(instruction.indexed_type));
            }
            store_slot_value(state, builder, instruction.result, llvm::ConstantInt::get(pointer_integer_type(), ordinal->second));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_const_enum const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol enum_type = state.routine->local_types.at(local_slot_index(instruction.target)).type;
            if (std::optional< quxlang::type_symbol > target = output_slot_target(enum_type); target.has_value())
            {
                enum_type = *target;
            }
            auto info_iterator = input.enum_infos.find(enum_type);
            if (info_iterator == input.enum_infos.end())
            {
                throw quxlang::lowering_compilation_error("INIT_ENUM destination has no enum_info: " + quxlang::to_string(enum_type));
            }
            auto case_iterator = info_iterator->second.values.find(instruction.case_name);
            if (case_iterator == info_iterator->second.values.end())
            {
                throw quxlang::compiler_bug("INIT_ENUM names unknown case '" + instruction.case_name + "' in " + quxlang::to_string(enum_type));
            }
            require_canonical_enum_value(info_iterator->second, case_iterator->second.value);
            std::uint32_t const bit_width = *nominal_integer_bit_width(enum_type);
            llvm::APInt const value = little_endian_apint(case_iterator->second.value, bit_width);
            store_slot_value(state, builder, instruction.target, llvm::ConstantInt::get(llvm::IntegerType::get(context, bit_width), value));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::enum_int_inrange const& instruction)
        {
            (void)current_block;
            auto info_iterator = input.enum_infos.find(instruction.enum_type);
            if (info_iterator == input.enum_infos.end())
            {
                throw quxlang::lowering_compilation_error("ENUM_INT_INRANGE references a type without enum_info: " + quxlang::to_string(instruction.enum_type));
            }
            llvm::Value* integer = integer_value(state, builder, instruction.integer);
            llvm::IntegerType* integer_type = llvm::cast< llvm::IntegerType >(integer->getType());
            llvm::Value* matches = llvm::ConstantInt::getFalse(context);
            for (std::map< std::string, quxlang::enum_value_info >::value_type const& entry : info_iterator->second.values)
            {
                require_canonical_enum_value(info_iterator->second, entry.second.value);
                llvm::APInt const case_bits = little_endian_apint(entry.second.value, integer_type->getBitWidth());
                llvm::Value* const equal = builder.CreateICmpEQ(integer, llvm::ConstantInt::get(integer_type, case_bits));
                matches = builder.CreateOr(matches, equal);
            }
            store_boolean(state, builder, instruction.result, matches);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::enum_cast const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol enum_type = state.routine->local_types.at(local_slot_index(instruction.result)).type;
            if (std::optional< quxlang::type_symbol > target = output_slot_target(enum_type); target.has_value())
            {
                enum_type = *target;
            }
            if (!input.enum_infos.contains(enum_type))
            {
                throw quxlang::lowering_compilation_error("ENUM_CAST destination has no enum_info: " + quxlang::to_string(enum_type));
            }
            llvm::Value* integer = integer_value(state, builder, instruction.integer);
            llvm::IntegerType* destination_type = llvm::cast< llvm::IntegerType >(value_storage_type(enum_type));
            store_slot_value(state, builder, instruction.result, integer_bits_to_width(builder, integer, destination_type));
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
            quxlang::type_symbol const value_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.source_reference)).type);
            llvm::Type* const llvm_value_type = value_storage_type(value_type);
            std::uint64_t byte_offset = inst.offset;
            if (module->getDataLayout().isBigEndian() && (llvm_value_type->isIntegerTy() || llvm_value_type->isFloatingPointTy()))
            {
                byte_offset = slot_size(value_type) - inst.offset - 1;
            }
            llvm::Value* pointer = load_reference_pointer(state, builder, inst.source_reference);
            llvm::Value* byte_pointer = builder.CreateInBoundsGEP(i8_type(), builder.CreateBitCast(pointer, opaque_pointer_type()), llvm::ConstantInt::get(i64_type(), byte_offset));
            llvm::Value* byte_value = builder.CreateLoad(i8_type(), byte_pointer);
            store_slot_value(state, builder, inst.result, byte_value);
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::set_value_byte const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::set_value_byte const& inst = instruction;
            quxlang::type_symbol const value_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.target_reference)).type);
            llvm::Type* const llvm_value_type = value_storage_type(value_type);
            std::uint64_t byte_offset = inst.offset;
            if (module->getDataLayout().isBigEndian() && (llvm_value_type->isIntegerTy() || llvm_value_type->isFloatingPointTy()))
            {
                byte_offset = slot_size(value_type) - inst.offset - 1;
            }
            llvm::Value* pointer = load_reference_pointer(state, builder, inst.target_reference);
            llvm::Value* byte_pointer = builder.CreateInBoundsGEP(i8_type(), builder.CreateBitCast(pointer, opaque_pointer_type()), llvm::ConstantInt::get(i64_type(), byte_offset));
            builder.CreateStore(load_slot_value(state, builder, inst.value), byte_pointer);
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::make_pointer_to const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::make_pointer_to const& inst = instruction;
            quxlang::type_symbol const& source_type = state.routine->local_types.at(local_slot_index(inst.of_index)).type;
            llvm::Value* pointer_value = quxlang::is_ref(source_type) ? load_reference_pointer(state, builder, inst.of_index) : value_address(state, inst.of_index);
            store_slot_value(state, builder, inst.pointer_index, pointer_value);
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::load_from_ref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::load_from_ref const& inst = instruction;
            std::map< quxlang::vmir2::local_index, bool >::const_iterator const fixed_cpu_attribute = state.fixed_cpu_attribute_references.find(inst.from_reference);
            if (fixed_cpu_attribute != state.fixed_cpu_attribute_references.end())
            {
                store_slot_value(state, builder, inst.to_value, llvm::ConstantInt::get(bool_storage_type(), fixed_cpu_attribute->second ? 1 : 0));
                return;
            }
            quxlang::type_symbol reference_type = state.routine->local_types.at(local_slot_index(inst.from_reference)).type;
            quxlang::type_symbol value_type = quxlang::remove_ref(reference_type);
            llvm::Value* pointer_value = load_reference_pointer(state, builder, inst.from_reference);
            llvm::LoadInst* load = builder.CreateLoad(value_storage_type(value_type), pointer_value);
            load->setAlignment(llvm::Align(slot_alignment(value_type)));
            if (std::optional< llvm::AtomicOrdering > const ordering = llvm_load_ordering(inst.access_mode); ordering.has_value())
            {
                llvm::Type* const storage_llvm_type = value_storage_type(value_type);
                if (storage_llvm_type->isIntegerTy() || storage_llvm_type->isPointerTy())
                {
                    std::uint64_t const storage_bits = storage_llvm_type->isPointerTy() ? input.machine_target.machine.pointer_size_bytes() * 8 : llvm::cast< llvm::IntegerType >(storage_llvm_type)->getBitWidth();
                    if (storage_bits > input.machine_target.machine.max_native_atomic_storage_bits())
                    {
                        throw quxlang::compiler_bug("Non-native atomic load lowering is not implemented for storage width " + std::to_string(storage_bits));
                    }
                }
                load->setAtomic(*ordering);
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

        /** Lowers direct-storage recovery as an address-preserving pointer conversion. */
        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::get_underyling_storage const& instruction)
        {
            (void)current_block;
            llvm::Value* pointer = load_slot_value(state, builder, instruction.object_pointer);
            store_slot_value(state, builder, instruction.storage_pointer, pointer);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::fusion_active_index const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const type = fusion_type(state, instruction.subject);
            quxlang::fusion_layout const& layout = input.fusion_layouts.at(type);
            llvm::Value* const tag = load_fusion_tag(fusion_object_pointer(state, instruction.subject), layout);
            llvm::IntegerType* const result_type = llvm::cast< llvm::IntegerType >(value_storage_type(state.routine->local_types.at(local_slot_index(instruction.result)).type));
            store_slot_value(state, builder, instruction.result, builder.CreateZExtOrTrunc(tag, result_type));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::fusion_has_alternative const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const type = fusion_type(state, instruction.subject);
            (void)fusion_alternative_type(type, instruction.alternative);
            quxlang::fusion_layout const& layout = input.fusion_layouts.at(type);
            llvm::Value* const tag = load_fusion_tag(fusion_object_pointer(state, instruction.subject), layout);
            llvm::IntegerType* const tag_type = llvm::cast< llvm::IntegerType >(tag->getType());
            store_boolean(state, builder, instruction.result, builder.CreateICmpEQ(tag, llvm::ConstantInt::get(tag_type, instruction.alternative)));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::fusion_is_valueless const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const type = fusion_type(state, instruction.subject);
            quxlang::fusion_layout const& layout = input.fusion_layouts.at(type);
            if (!layout.valueless_tag.has_value())
            {
                store_boolean(state, builder, instruction.result, llvm::ConstantInt::getFalse(context));
                return;
            }
            llvm::Value* const tag = load_fusion_tag(fusion_object_pointer(state, instruction.subject), layout);
            llvm::IntegerType* const tag_type = llvm::cast< llvm::IntegerType >(tag->getType());
            store_boolean(state, builder, instruction.result, builder.CreateICmpEQ(tag, llvm::ConstantInt::get(tag_type, *layout.valueless_tag)));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::fusion_storage_ref const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const type = fusion_type(state, instruction.subject);
            quxlang::type_symbol const alternative_type = fusion_alternative_type(type, instruction.alternative);
            if (alternative_type.type_is< quxlang::void_type >())
            {
                throw quxlang::semantic_compilation_error("FUSION_STORAGE_REF cannot project a VOID alternative");
            }
            quxlang::fusion_layout const& layout = input.fusion_layouts.at(type);
            llvm::Value* const object_pointer = fusion_object_pointer(state, instruction.subject);
            llvm::Value* payload_pointer = nullptr;
            if (layout.is_inline)
            {
                payload_pointer = fusion_field_pointer(object_pointer, layout.payload_offset);
            }
            else
            {
                payload_pointer = builder.CreateLoad(opaque_pointer_type(), fusion_field_pointer(object_pointer, layout.payload_offset));
            }
            store_reference_pointer(state, builder, instruction.result, payload_pointer);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::fusion_set_active const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const type = fusion_type(state, instruction.target);
            quxlang::type_symbol const alternative_type = fusion_alternative_type(type, instruction.alternative);
            quxlang::fusion_layout const& layout = input.fusion_layouts.at(type);
            llvm::Value* const object_pointer = fusion_object_pointer(state, instruction.target);

            if (layout.is_inline)
            {
                if (instruction.payload_storage.has_value())
                {
                    throw quxlang::semantic_compilation_error("Inline FUSION_SET_ACTIVE does not accept external payload storage");
                }
            }
            else
            {
                llvm::Value* payload_pointer = llvm::ConstantPointerNull::get(opaque_pointer_type());
                if (!alternative_type.type_is< quxlang::void_type >())
                {
                    if (!instruction.payload_storage.has_value())
                    {
                        throw quxlang::semantic_compilation_error("Boxed non-VOID FUSION_SET_ACTIVE requires payload storage");
                    }
                    quxlang::type_symbol const& payload_slot_type = state.routine->local_types.at(local_slot_index(*instruction.payload_storage)).type;
                    payload_pointer = quxlang::is_ref(payload_slot_type) ? load_reference_pointer(state, builder, *instruction.payload_storage) : load_slot_value(state, builder, *instruction.payload_storage);
                }
                else if (instruction.payload_storage.has_value())
                {
                    throw quxlang::semantic_compilation_error("VOID FUSION_SET_ACTIVE cannot accept payload storage");
                }
                builder.CreateStore(payload_pointer, fusion_field_pointer(object_pointer, layout.payload_offset));
            }

            store_fusion_tag(object_pointer, layout, instruction.alternative);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::fusion_set_valueless const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const type = fusion_type(state, instruction.target);
            quxlang::fusion_layout const& layout = input.fusion_layouts.at(type);
            if (!layout.valueless_tag.has_value())
            {
                throw quxlang::semantic_compilation_error("FUSION_SET_VALUELESS cannot be used with NEVER_VALUELESS fusion");
            }
            llvm::Value* const object_pointer = fusion_object_pointer(state, instruction.target);
            if (!layout.is_inline)
            {
                builder.CreateStore(llvm::ConstantPointerNull::get(opaque_pointer_type()), fusion_field_pointer(object_pointer, layout.payload_offset));
            }
            store_fusion_tag(object_pointer, layout, *layout.valueless_tag);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::fusion_swap_boxed_state const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const a_type = fusion_type(state, instruction.a);
            quxlang::type_symbol const b_type = fusion_type(state, instruction.b);
            if (a_type != b_type || input.fusion_layouts.at(a_type).is_inline)
            {
                throw quxlang::semantic_compilation_error("FUSION_SWAP_BOXED_STATE requires the same boxed fusion type");
            }
            quxlang::fusion_layout const& layout = input.fusion_layouts.at(a_type);
            llvm::Value* const a_object = fusion_object_pointer(state, instruction.a);
            llvm::Value* const b_object = fusion_object_pointer(state, instruction.b);
            llvm::Value* const a_payload_address = fusion_field_pointer(a_object, layout.payload_offset);
            llvm::Value* const b_payload_address = fusion_field_pointer(b_object, layout.payload_offset);
            llvm::Value* const a_tag_address = fusion_field_pointer(a_object, layout.tag_offset);
            llvm::Value* const b_tag_address = fusion_field_pointer(b_object, layout.tag_offset);
            llvm::Value* const a_payload = builder.CreateLoad(opaque_pointer_type(), a_payload_address);
            llvm::Value* const b_payload = builder.CreateLoad(opaque_pointer_type(), b_payload_address);
            llvm::Type* const tag_type = value_storage_type(layout.tag_type);
            llvm::Value* const a_tag = builder.CreateLoad(tag_type, a_tag_address);
            llvm::Value* const b_tag = builder.CreateLoad(tag_type, b_tag_address);
            builder.CreateStore(b_payload, a_payload_address);
            builder.CreateStore(a_payload, b_payload_address);
            builder.CreateStore(b_tag, a_tag_address);
            builder.CreateStore(a_tag, b_tag_address);
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

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::jvm_allocate_object_storage const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("JVM_ALLOCATE_OBJECT_STORAGE cannot be lowered to native code");
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::jvm_allocate_multiple_object_storage const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("JVM_ALLOCATE_OBJECT_STORAGE with a count cannot be lowered to native code");
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::jvm_deallocate_object_storage const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("JVM_DEALLOCATE_OBJECT_STORAGE cannot be lowered to native code");
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::jvm_deallocate_multiple_object_storage const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("JVM_DEALLOCATE_OBJECT_STORAGE with a count cannot be lowered to native code");
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::jvm_gc_pointer_checked_cast const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            throw quxlang::lowering_compilation_error("checked JVM GC-pointer casts cannot be lowered to native code");
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::get_object_ref const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::get_object_ref const& inst = instruction;
            llvm::GlobalVariable* global = nullptr;
            switch (inst.type)
            {
            case quxlang::vmir2::access_type::storage:
            case quxlang::vmir2::access_type::object:
                if (std::map< quxlang::type_symbol, llvm::GlobalVariable* >::const_iterator constant = constant_globals.find(inst.symbol); constant != constant_globals.end())
                {
                    store_reference_pointer(state, builder, inst.target_ref, constant->second);
                    return;
                }
                std::map< quxlang::type_symbol, llvm::GlobalVariable* >::iterator object = mutable_globals.find(inst.symbol);
                if (object == mutable_globals.end())
                {
                    quxlang::type_symbol target_type = quxlang::remove_ref(state.routine->local_types.at(local_slot_index(inst.target_ref)).type);
                    global = get_or_create_common_zero_initialized_global(inst.symbol, value_storage_type(target_type));
                }
                else
                {
                    global = object->second;
                }
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

        /** Lowers one allocation-free current-thread destructor registration. */
        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::thread_destructor_register const& instruction)
        {
            (void)state;
            (void)current_block;
            quxlang::type_symbol const node_symbol = quxlang::subsymbol{
                .of = instruction.symbol,
                .name = "THREAD_DESTRUCTOR_NODE",
            };
            llvm::GlobalVariable* const node = get_or_create_common_zero_initialized_global(
                node_symbol,
                value_storage_type(quxlang::llvm_backend::runtime_thread_destructor_node_type()));
            apply_access_class(node, quxlang::vmir2::access_class::thread);
            llvm::GlobalVariable* const guard = get_or_create_initguard_global(instruction.symbol, quxlang::vmir2::access_class::thread);

            if (!instruction.deinitializer.type_is< quxlang::instanciation_reference >())
            {
                throw quxlang::semantic_compilation_error("Thread-local deinitializer is not a concrete procedure: " + quxlang::to_string(instruction.deinitializer));
            }
            callable_abi const deinitializer_abi = callable_abi_from_instanciation_reference(
                instruction.deinitializer.get_as< quxlang::instanciation_reference >(), std::nullopt);
            llvm::Function* const deinitializer = get_or_create_external_function(instruction.deinitializer, deinitializer_abi);

            quxlang::llvm_backend::runtime_procedure_reference const reference{
                .procedure = quxlang::llvm_backend::runtime_procedure::thread_destructor_register,
            };
            quxlang::type_symbol const& registration_symbol = runtime_procedure_symbol(reference);
            if (!registration_symbol.type_is< quxlang::instanciation_reference >())
            {
                throw quxlang::semantic_compilation_error("Thread destructor registration runtime procedure is not concrete");
            }
            callable_abi const registration_abi = callable_abi_from_instanciation_reference(
                registration_symbol.get_as< quxlang::instanciation_reference >(), std::nullopt);
            llvm::Function* const registration = get_or_create_external_function(registration_symbol, registration_abi);
            std::vector< llvm::Value* > arguments;
            arguments.reserve(registration_abi.llvm_param_source_indices.size());
            for (std::size_t const source_index : registration_abi.llvm_param_source_indices)
            {
                abi_parameter const& parameter = registration_abi.source_ordered.at(source_index);
                if (!parameter.name.has_value())
                {
                    throw quxlang::semantic_compilation_error("Thread destructor registration runtime parameters must be named");
                }
                if (*parameter.name == "node")
                {
                    arguments.push_back(node);
                }
                else if (*parameter.name == "guard")
                {
                    arguments.push_back(guard);
                }
                else if (*parameter.name == "deinitializer")
                {
                    arguments.push_back(deinitializer);
                }
                else
                {
                    throw quxlang::semantic_compilation_error("Unknown thread destructor registration runtime parameter: " + *parameter.name);
                }
            }
            llvm::CallInst* const call = builder.CreateCall(registration_abi.llvm_type, registration, arguments);
            apply_calling_convention(call, registration_abi);
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
            store->setAlignment(llvm::Align(slot_alignment(destination_type)));
            if (std::optional< llvm::AtomicOrdering > const ordering = llvm_store_ordering(inst.access_mode); ordering.has_value())
            {
                llvm::Type* const storage_llvm_type = value_storage_type(destination_type);
                if (storage_llvm_type->isIntegerTy() || storage_llvm_type->isPointerTy())
                {
                    std::uint64_t const storage_bits = storage_llvm_type->isPointerTy() ? input.machine_target.machine.pointer_size_bytes() * 8 : llvm::cast< llvm::IntegerType >(storage_llvm_type)->getBitWidth();
                    if (storage_bits > input.machine_target.machine.max_native_atomic_storage_bits())
                    {
                        throw quxlang::compiler_bug("Non-native atomic store lowering is not implemented for storage width " + std::to_string(storage_bits));
                    }
                }
                store->setAtomic(*ordering);
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
                std::uint64_t const storage_bits = storage_type->isPointerTy() ? input.machine_target.machine.pointer_size_bytes() * 8 : llvm::cast< llvm::IntegerType >(storage_type)->getBitWidth();
                if (storage_bits > input.machine_target.machine.max_native_atomic_storage_bits())
                {
                    throw quxlang::compiler_bug("Non-native atomic compare_exchange lowering is not implemented for storage width " + std::to_string(storage_bits));
                }

                llvm::Value* expected_value = builder.CreateLoad(value_storage_type(target_type), expected_pointer);
                expected_value = logical_atomic_value_to_storage(builder, atomic_type, expected_value);
                desired_value = logical_atomic_value_to_storage(builder, atomic_type, desired_value);
                llvm::AtomicCmpXchgInst* cmpxchg = builder.CreateAtomicCmpXchg(target_pointer, expected_value, desired_value, llvm::Align(slot_alignment(atomic_type)), llvm_cmpxchg_success_ordering(inst.success_mode), llvm_cmpxchg_failure_ordering(inst.failure_mode));
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
            (void)fixed_integer_binary_type(state, instruction.a, instruction.b, instruction.result, "IADD");
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateAdd(lhs, rhs));
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_mul const& instruction)
        {
            (void)current_block;
            (void)fixed_integer_binary_type(state, instruction.a, instruction.b, instruction.result, "IMUL");
            llvm::Value* lhs = integer_value(state, builder, instruction.a);
            llvm::Value* rhs = integer_value(state, builder, instruction.b);
            store_slot_value(state, builder, instruction.result, builder.CreateMul(lhs, rhs));
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_div const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = fixed_integer_binary_type(state, instruction.a, instruction.b, instruction.result, "IDIV");
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
            quxlang::type_symbol const& type = fixed_integer_binary_type(state, instruction.a, instruction.b, instruction.result, "IMOD");
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
            (void)fixed_integer_binary_type(state, instruction.a, instruction.b, instruction.result, "ISUB");
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

        /** Stores the canonical ORDER value selected by mutually exclusive less/greater conditions. */
        void store_comparison_order(function_codegen_state& state, quxlang::vmir2::local_index result, llvm::Value* less, llvm::Value* greater)
        {
            if (state.routine->local_types.at(local_slot_index(result)).type != quxlang::type_symbol(quxlang::builtin_symbol{"ORDER"}))
            {
                throw quxlang::lowering_compilation_error("Comparison result must have type ORDER");
            }
            llvm::Value* const less_value = llvm::ConstantInt::getSigned(i8_type(), -1);
            llvm::Value* const equal_value = llvm::ConstantInt::get(i8_type(), 0);
            llvm::Value* const greater_value = llvm::ConstantInt::get(i8_type(), 1);
            llvm::Value* const non_less_value = builder.CreateSelect(greater, greater_value, equal_value);
            store_slot_value(state, builder, result, builder.CreateSelect(less, less_value, non_less_value));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::int_cmp const& instruction)
        {
            (void)current_block;
            quxlang::type_symbol const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            bool is_signed = true;
            if (type.type_is< quxlang::int_type >())
            {
                is_signed = type.get_as< quxlang::int_type >().has_sign;
            }
            else if (type.type_is< quxlang::size_type >() || (nominal_integer_runtime_type(type) && !nominal_integer_is_signed(type)))
            {
                is_signed = false;
            }
            llvm::Value* const less = is_signed ? builder.CreateICmpSLT(lhs, rhs) : builder.CreateICmpULT(lhs, rhs);
            llvm::Value* const greater = is_signed ? builder.CreateICmpSGT(lhs, rhs) : builder.CreateICmpUGT(lhs, rhs);
            store_comparison_order(state, instruction.result, less, greater);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::float_cmp const& instruction)
        {
            (void)current_block;
            quxlang::float_type const& type = state.routine->local_types.at(local_slot_index(instruction.a)).type.get_as< quxlang::float_type >();
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            llvm::Value* lhs_key = float_total_order_key(lhs, type, builder);
            llvm::Value* rhs_key = float_total_order_key(rhs, type, builder);
            store_comparison_order(state, instruction.result, builder.CreateICmpULT(lhs_key, rhs_key), builder.CreateICmpUGT(lhs_key, rhs_key));
        }

        /** Emits an unsigned address comparison for ADDRESS, pointer, and global comparison instructions. */
        void emit_address_comparison(function_codegen_state& state, quxlang::vmir2::local_index a, quxlang::vmir2::local_index b, quxlang::vmir2::local_index result)
        {
            llvm::Value* lhs = builder.CreatePtrToInt(load_slot_value(state, builder, a), pointer_integer_type());
            llvm::Value* rhs = builder.CreatePtrToInt(load_slot_value(state, builder, b), pointer_integer_type());
            store_comparison_order(state, result, builder.CreateICmpULT(lhs, rhs), builder.CreateICmpUGT(lhs, rhs));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::address_cmp const& instruction)
        {
            (void)current_block;
            emit_address_comparison(state, instruction.a, instruction.b, instruction.result);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::type_index_cmp const& instruction)
        {
            (void)current_block;
            llvm::Value* lhs = load_slot_value(state, builder, instruction.a);
            llvm::Value* rhs = load_slot_value(state, builder, instruction.b);
            store_comparison_order(state, instruction.result, builder.CreateICmpULT(lhs, rhs), builder.CreateICmpUGT(lhs, rhs));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pointer_cmp const& instruction)
        {
            (void)current_block;
            emit_address_comparison(state, instruction.a, instruction.b, instruction.result);
        }

        /** Emits pointer/global equality without imposing an ordering requirement. */
        void emit_address_equality(function_codegen_state& state, quxlang::vmir2::local_index a, quxlang::vmir2::local_index b, quxlang::vmir2::local_index result, llvm::CmpInst::Predicate predicate)
        {
            llvm::Value* lhs = load_slot_value(state, builder, a);
            llvm::Value* rhs = load_slot_value(state, builder, b);
            store_boolean(state, builder, result, builder.CreateICmp(predicate, lhs, rhs));
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pointer_eq const& instruction)
        {
            (void)current_block;
            emit_address_equality(state, instruction.a, instruction.b, instruction.result, llvm::CmpInst::ICMP_EQ);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::pointer_ne const& instruction)
        {
            (void)current_block;
            emit_address_equality(state, instruction.a, instruction.b, instruction.result, llvm::CmpInst::ICMP_NE);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::global_cmp const& instruction)
        {
            (void)current_block;
            emit_address_comparison(state, instruction.a, instruction.b, instruction.result);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::global_eq const& instruction)
        {
            (void)current_block;
            emit_address_equality(state, instruction.a, instruction.b, instruction.result, llvm::CmpInst::ICMP_EQ);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::global_ne const& instruction)
        {
            (void)current_block;
            emit_address_equality(state, instruction.a, instruction.b, instruction.result, llvm::CmpInst::ICMP_NE);
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::cmp_bool const& instruction)
        {
            (void)current_block;
            if (state.routine->local_types.at(local_slot_index(instruction.ordering)).type != quxlang::type_symbol(quxlang::builtin_symbol{"ORDER"}))
            {
                throw quxlang::lowering_compilation_error("CMP_BOOL input must have type ORDER");
            }
            llvm::Value* const ordering = load_slot_value(state, builder, instruction.ordering);
            llvm::Value* const zero = llvm::ConstantInt::get(i8_type(), 0);
            llvm::Value* result = nullptr;
            switch (instruction.relation)
            {
            case quxlang::vmir2::comparison_relation::equal:
                result = builder.CreateICmpEQ(ordering, zero);
                break;
            case quxlang::vmir2::comparison_relation::not_equal:
                result = builder.CreateICmpNE(ordering, zero);
                break;
            case quxlang::vmir2::comparison_relation::less:
                result = builder.CreateICmpSLT(ordering, zero);
                break;
            case quxlang::vmir2::comparison_relation::less_equal:
                result = builder.CreateICmpSLE(ordering, zero);
                break;
            case quxlang::vmir2::comparison_relation::greater:
                result = builder.CreateICmpSGT(ordering, zero);
                break;
            case quxlang::vmir2::comparison_relation::greater_equal:
                result = builder.CreateICmpSGE(ordering, zero);
                break;
            }
            store_boolean(state, builder, instruction.result, result);
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
            std::map< quxlang::type_symbol, quxlang::struct_layout >::const_iterator layout_iter = input.struct_layouts.find(base_type);
            if (layout_iter == input.struct_layouts.end())
            {
                throw quxlang::semantic_compilation_error("Missing struct layout for STRUCT_INIT_START on " + quxlang::to_string(base_type));
            }
            for (quxlang::vmir2::struct_init_delegate const& delegate : inst.delegates)
            {
                std::int64_t subobject_offset = 0;
                if (quxlang::typeis< quxlang::vmir2::struct_init_field_selector >(delegate.selector))
                {
                    std::size_t const field_ordinal = quxlang::as< quxlang::vmir2::struct_init_field_selector >(delegate.selector).field_ordinal;
                    std::vector< quxlang::struct_field_info >::const_iterator const field = std::ranges::find_if(layout_iter->second.fields, [field_ordinal](quxlang::struct_field_info const& candidate)
                    {
                        return candidate.declaration_ordinal == field_ordinal;
                    });
                    if (field == layout_iter->second.fields.end())
                    {
                        throw quxlang::compiler_bug("STRUCT_INIT_START names an unknown field ordinal");
                    }
                    subobject_offset = static_cast< std::int64_t >(field->offset);
                }
                else if (quxlang::typeis< quxlang::vmir2::struct_init_direct_base_selector >(delegate.selector))
                {
                    std::size_t const direct_base_ordinal = quxlang::as< quxlang::vmir2::struct_init_direct_base_selector >(delegate.selector).direct_base_ordinal;
                    std::vector< quxlang::struct_base_layout_info >::const_iterator const base = std::ranges::find_if(layout_iter->second.direct_bases, [direct_base_ordinal](quxlang::struct_base_layout_info const& candidate)
                    {
                        return candidate.declaration_ordinal == direct_base_ordinal;
                    });
                    if (base == layout_iter->second.direct_bases.end() || base->kind == quxlang::inheritance_kind::virtual_)
                    {
                        throw quxlang::compiler_bug("STRUCT_INIT_START names an unknown nonvirtual direct-base ordinal");
                    }
                    subobject_offset = base->offset;
                }
                else
                {
                    std::size_t const virtual_base_ordinal = quxlang::as< quxlang::vmir2::struct_init_virtual_base_selector >(delegate.selector).virtual_base_ordinal;
                    if (virtual_base_ordinal >= layout_iter->second.virtual_bases.size())
                    {
                        throw quxlang::compiler_bug("STRUCT_INIT_START names an unknown virtual-base ordinal");
                    }
                    subobject_offset = layout_iter->second.virtual_bases.at(virtual_base_ordinal).offset;
                }
                llvm::Value* byte_pointer = builder.CreateBitCast(base_pointer, opaque_pointer_type());
                llvm::Value* subobject_pointer = builder.CreateInBoundsGEP(i8_type(), byte_pointer, llvm::ConstantInt::getSigned(i64_type(), subobject_offset));
                assign_slot_alias(state, delegate.value, subobject_pointer);
            }
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::struct_init_finish const& instruction)
        {
            (void)state;
            (void)current_block;
            (void)instruction;
            return;
        }

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::copy_reference const& instruction)
        {
            (void)current_block;
            quxlang::vmir2::copy_reference const& inst = instruction;
            store_reference_pointer(state, builder, inst.to_index, load_reference_pointer(state, builder, inst.from_index));
            std::map< quxlang::vmir2::local_index, bool >::const_iterator const fixed_cpu_attribute = state.fixed_cpu_attribute_references.find(inst.from_index);
            if (fixed_cpu_attribute != state.fixed_cpu_attribute_references.end())
            {
                state.fixed_cpu_attribute_references.emplace(inst.to_index, fixed_cpu_attribute->second);
            }
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
            if (array_type.element_type.type_is< quxlang::procedure_type >())
            {
                llvm::Value* procedure_pointer = builder.CreateLoad(opaque_pointer_type(), element_pointer);
                store_reference_pointer(state, builder, inst.store_index, procedure_pointer);
                return;
            }
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
            if (element_type.type_is< quxlang::procedure_type >())
            {
                llvm::Value* procedure_pointer = builder.CreateLoad(opaque_pointer_type(), element_pointer);
                store_reference_pointer(state, builder, inst.store_index, procedure_pointer);
                return;
            }
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
            if (pointee_type.type_is< quxlang::address_type >())
            {
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::get(i64_type(), 1));
                builder.CreateStore(updated_pointer, target_pointer);
                store_slot_value(state, builder, instruction.result, old_pointer);
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
            if (pointee_type.type_is< quxlang::address_type >())
            {
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::getSigned(i64_type(), -1));
                builder.CreateStore(updated_pointer, target_pointer);
                store_slot_value(state, builder, instruction.result, old_pointer);
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
            if (pointee_type.type_is< quxlang::address_type >())
            {
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::get(i64_type(), 1));
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
            if (pointee_type.type_is< quxlang::address_type >())
            {
                llvm::Value* old_pointer = builder.CreateLoad(opaque_pointer_type(), target_pointer);
                llvm::Value* updated_pointer = builder.CreateInBoundsGEP(i8_type(), old_pointer, llvm::ConstantInt::getSigned(i64_type(), -1));
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
            std::uint64_t element_size;
            if (pointer_type.type_is< quxlang::address_type >())
            {
                element_size = 1;
            }
            else
            {
                quxlang::type_symbol element_type = quxlang::remove_ptr(pointer_type);
                element_size = slot_size(element_type);
            }
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
            llvm::Value* byte_offset = builder.CreateMul(index_value, llvm::ConstantInt::get(i64_type(), element_size));
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

            std::uint64_t element_size;
            if (from_type.type_is< quxlang::address_type >() || to_type.type_is< quxlang::address_type >())
            {
                if (!(from_type.type_is< quxlang::address_type >() && to_type.type_is< quxlang::address_type >()))
                {
                    throw quxlang::semantic_compilation_error("PTR_DIFF requires both operands to be ADDRESS, or both to be matching pointer types");
                }
                element_size = 1;
            }
            else
            {
                if (!quxlang::is_ptr(from_type) || !quxlang::is_ptr(to_type))
                {
                    throw quxlang::semantic_compilation_error("PTR_DIFF requires pointer operands for LLVM lowering");
                }

                quxlang::type_symbol const element_type = quxlang::remove_ptr(from_type);
                if (element_type != quxlang::remove_ptr(to_type))
                {
                    throw quxlang::semantic_compilation_error("PTR_DIFF requires matching pointer element types for LLVM lowering");
                }
                element_size = slot_size(element_type);
            }

            llvm::Value* from_pointer = load_slot_value(state, builder, inst.from);
            llvm::Value* to_pointer = load_slot_value(state, builder, inst.to);
            llvm::Value* from_address = builder.CreatePtrToInt(from_pointer, i64_type());
            llvm::Value* to_address = builder.CreatePtrToInt(to_pointer, i64_type());
            llvm::Value* element_delta = builder.CreateSub(from_address, to_address);
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

        void emit_instruction_ovl(function_codegen_state& state, llvm::BasicBlock*& current_block, quxlang::vmir2::lowering_error const& instruction)
        {
            (void)state;
            (void)current_block;
            throw quxlang::lowering_compilation_error(instruction.message);
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
                llvm::BasicBlock* edge_target = cleanup_edge_target(state, current_block, state.current_state, state.routine->blocks.at(block_slot_index(inst.target)).entry_state, target_block);
                builder.CreateBr(edge_target);
                return;
            }
            if (terminator.type_is< quxlang::vmir2::branch >())
            {
                quxlang::vmir2::branch const& inst = terminator.as< quxlang::vmir2::branch >();
                llvm::BasicBlock* true_target = cleanup_edge_target(state, current_block, state.current_state, state.routine->blocks.at(block_slot_index(inst.target_true)).entry_state, state.blocks.at(inst.target_true));
                llvm::BasicBlock* false_target = cleanup_edge_target(state, current_block, state.current_state, state.routine->blocks.at(block_slot_index(inst.target_false)).entry_state, state.blocks.at(inst.target_false));
                builder.CreateCondBr(truth_value(state, builder, inst.condition), true_target, false_target);
                return;
            }
            if (terminator.type_is< quxlang::vmir2::tablebranch >())
            {
                quxlang::vmir2::tablebranch const& inst = terminator.as< quxlang::vmir2::tablebranch >();
                llvm::Value* const ordinal = load_slot_value(state, builder, inst.index);
                if (!ordinal->getType()->isIntegerTy())
                {
                    throw quxlang::semantic_compilation_error("TABLEBRANCH index must have integer storage");
                }

                llvm::BasicBlock* const default_target = cleanup_edge_target(state, current_block, state.current_state, state.routine->blocks.at(block_slot_index(inst.default_target)).entry_state, state.blocks.at(inst.default_target));
                llvm::SwitchInst* const switch_instruction = builder.CreateSwitch(ordinal, default_target, static_cast< unsigned >(inst.targets.size()));
                llvm::IntegerType* const ordinal_type = llvm::cast< llvm::IntegerType >(ordinal->getType());
                for (std::size_t i = 0; i < inst.targets.size(); ++i)
                {
                    quxlang::vmir2::block_index const target = inst.targets[i];
                    llvm::BasicBlock* const edge_target = cleanup_edge_target(state, current_block, state.current_state, state.routine->blocks.at(block_slot_index(target)).entry_state, state.blocks.at(target));
                    switch_instruction->addCase(llvm::ConstantInt::get(ordinal_type, i), edge_target);
                }
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
            if (terminator.type_is< quxlang::vmir2::unreachable >())
            {
                builder.CreateUnreachable();
                return;
            }
            if (terminator.type_is< quxlang::vmir2::panic >())
            {
                quxlang::vmir2::panic const& inst = terminator.as< quxlang::vmir2::panic >();
                quxlang::vmir2::source_index const empty_source_index;
                quxlang::vmir2::source_index const& source_index = input.source_index.has_value() ? input.source_index->get() : empty_source_index;
                quxlang::llvm_backend::runtime_procedure_reference const panic_reference{.procedure = quxlang::llvm_backend::runtime_procedure::panic};
                quxlang::type_symbol const& panic_symbol = runtime_procedure_symbol(panic_reference);
                if (!panic_symbol.type_is< quxlang::instanciation_reference >())
                {
                    throw quxlang::semantic_compilation_error("Runtime PANIC did not initialize to a concrete procedure: " + quxlang::to_string(panic_symbol));
                }
                callable_abi const panic_abi = callable_abi_from_instanciation_reference(panic_symbol.get_as< quxlang::instanciation_reference >(), std::nullopt);
                llvm::Function* const panic_function = get_or_create_external_function(panic_symbol, panic_abi);
                quxlang::llvm_backend::runtime_panic_call_arguments const panic_arguments = quxlang::llvm_backend::runtime_panic_arguments(inst, source_index);
                llvm::CallInst* const call = builder.CreateCall(panic_abi.llvm_type, panic_function, runtime_panic_call_arguments(panic_arguments, panic_abi));
                apply_calling_convention(call, panic_abi);
                builder.CreateUnreachable();
                return;
            }
            if (terminator.type_is< quxlang::vmir2::runtime_constexpr >())
            {
                quxlang::vmir2::runtime_constexpr const& inst = terminator.as< quxlang::vmir2::runtime_constexpr >();
                llvm::BasicBlock* target_block = state.blocks.at(inst.target_native);
                llvm::BasicBlock* edge_target = cleanup_edge_target(state, current_block, state.current_state, state.routine->blocks.at(block_slot_index(inst.target_native)).entry_state, target_block);
                builder.CreateBr(edge_target);
                return;
            }
            if (terminator.type_is< quxlang::vmir2::initguard_try_acquire >())
            {
                quxlang::vmir2::initguard_try_acquire const& inst = terminator.as< quxlang::vmir2::initguard_try_acquire >();
                llvm::Value* guard_pointer = get_or_create_initguard_global(inst.symbol, inst.class_);
                quxlang::llvm_backend::runtime_procedure const procedure = inst.class_ == quxlang::vmir2::access_class::thread
                    ? quxlang::llvm_backend::runtime_procedure::thread_initguard_try_acquire
                    : quxlang::llvm_backend::runtime_procedure::initguard_try_acquire;
                callable_abi const try_acquire_abi = initguard_runtime_abi(procedure);
                llvm::Function* try_acquire = get_or_create_initguard_runtime_function(procedure, try_acquire_abi);
                llvm::CallInst* try_acquire_call = builder.CreateCall(try_acquire_abi.llvm_type, try_acquire, {guard_pointer});
                apply_calling_convention(try_acquire_call, try_acquire_abi);
                llvm::Value* acquired = builder.CreateICmpNE(try_acquire_call, llvm::ConstantInt::get(llvm::cast< llvm::IntegerType >(try_acquire_call->getType()), 0));
                llvm::BasicBlock* acquired_block = state.blocks.at(inst.target_acquired);
                llvm::BasicBlock* initialized_block = state.blocks.at(inst.target_already_initialized);
                llvm::BasicBlock* initialized_edge = cleanup_edge_target(state, current_block, state.current_state, state.routine->blocks.at(block_slot_index(inst.target_already_initialized)).entry_state, initialized_block);

                quxlang::vmir2::state_map acquired_state = state.current_state;
                acquired_state[inst.target_lock].stage = quxlang::vmir2::slot_stage::full;
                acquired_state[inst.target_lock].storage_valid = true;
                llvm::BasicBlock* acquired_store_block = llvm::BasicBlock::Create(context, "initguard.acquired", state.function);
                llvm::BasicBlock* acquired_edge = cleanup_edge_target(state, acquired_store_block, acquired_state, state.routine->blocks.at(block_slot_index(inst.target_acquired)).entry_state, acquired_block);
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
            for (quxlang::vmir2::block_index const block : quxlang::vmir2::reachable_blocks(routine, quxlang::dependency_set::native))
            {
                native_reachable_blocks.at(block_slot_index(block)) = true;
            }
            std::set< quxlang::vmir2::local_index > const native_reachable_locals = quxlang::vmir2::reachable_local_slots(routine, quxlang::dependency_set::native);

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
                if (!native_reachable_locals.contains(quxlang::vmir2::local_index(i)))
                {
                    continue;
                }
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
                    emit_instruction(state, current_block, inst);
                    state_engine.apply(inst);
                    emit_post_instruction_array_initializer_progress(state, builder, previous_state, state.current_state);
                    emit_post_instruction_poison_cleanup(state, builder, previous_state, state.current_state, inst);
                }

                if (current_block->getTerminator() != nullptr)
                {
                    continue;
                }

                if (block.terminator.has_value())
                {
                    emit_terminator(state, current_block, *block.terminator);
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

auto quxlang::llvm_backend::llvm_compilation_target_for_stepping(quxlang::machine_target_info const& machine, quxlang::llvm_backend::optimization_level optimization, quxlang::cpu_stepping_configuration const& stepping) -> quxlang::llvm_backend::llvm_compilation_target
{
    quxlang::llvm_backend::llvm_compilation_target result{
        .machine = machine,
        .optimization = optimization,
    };
    for (std::pair< std::string const, bool > const& attribute_setting : stepping.attributes)
    {
        std::map< std::string, quxlang::cpu_attribute_group >::const_iterator const group = quxlang::cpu_attribute_groups.find(attribute_setting.first);
        if (group == quxlang::cpu_attribute_groups.end())
        {
            std::pair< std::map< std::string, bool >::iterator, bool > const insertion = result.fixed_cpu_attribute_values.emplace(attribute_setting);
            if (!insertion.second && insertion.first->second != attribute_setting.second)
            {
                throw quxlang::semantic_compilation_error("CPU stepping contains conflicting attribute constraints: " + attribute_setting.first);
            }
            continue;
        }
        if (group->second.cpu_type != machine.cpu_type)
        {
            throw quxlang::semantic_compilation_error("CPU stepping attribute does not apply to the LLVM target: " + attribute_setting.first);
        }
        if (!attribute_setting.second)
        {
            continue;
        }
        for (std::string const& group_attribute : group->second.attributes)
        {
            std::pair< std::map< std::string, bool >::iterator, bool > const insertion = result.fixed_cpu_attribute_values.emplace(group_attribute, true);
            if (!insertion.second && !insertion.first->second)
            {
                throw quxlang::semantic_compilation_error("CPU stepping contains conflicting attribute constraints: " + group_attribute);
            }
        }
    }
    if (optimization != quxlang::llvm_backend::optimization_level::release)
    {
        return result;
    }

    if (stepping.tune.has_value())
    {
        std::optional< quxlang::cpu_tuning_model_entry > const parsed_tune = quxlang::parse_cpu_tuning_model(*stepping.tune);
        if (!parsed_tune.has_value())
        {
            throw quxlang::semantic_compilation_error("Unknown CPU tuning model: " + *stepping.tune);
        }
        if (parsed_tune->cpu_type != machine.cpu_type)
        {
            throw quxlang::semantic_compilation_error("CPU tuning model does not apply to the LLVM target: " + *stepping.tune);
        }

        switch (parsed_tune->model)
        {
        case quxlang::cpu_tuning_model::x86_amd_k6:
            result.tune_cpu = "k6";
            break;
        case quxlang::cpu_tuning_model::x86_amd_k6_2:
            result.tune_cpu = "k6-2";
            break;
        case quxlang::cpu_tuning_model::x86_amd_athlon:
            result.tune_cpu = "athlon";
            break;
        case quxlang::cpu_tuning_model::x86_amd_athlon_xp:
            result.tune_cpu = "athlon-xp";
            break;
        case quxlang::cpu_tuning_model::x86_amd_geode:
            result.tune_cpu = "geode";
            break;
        case quxlang::cpu_tuning_model::x64_amd_k8:
            result.tune_cpu = "k8";
            break;
        case quxlang::cpu_tuning_model::x64_amd_k10:
            result.tune_cpu = "amdfam10";
            break;
        case quxlang::cpu_tuning_model::x64_amd_bobcat:
            result.tune_cpu = "btver1";
            break;
        case quxlang::cpu_tuning_model::x64_amd_jaguar:
            result.tune_cpu = "btver2";
            break;
        case quxlang::cpu_tuning_model::x64_amd_bulldozer:
            result.tune_cpu = "bdver1";
            break;
        case quxlang::cpu_tuning_model::x64_amd_piledriver:
            result.tune_cpu = "bdver2";
            break;
        case quxlang::cpu_tuning_model::x64_amd_steamroller:
            result.tune_cpu = "bdver3";
            break;
        case quxlang::cpu_tuning_model::x64_amd_excavator:
            result.tune_cpu = "bdver4";
            break;
        case quxlang::cpu_tuning_model::x64_amd_zen1:
            result.tune_cpu = "znver1";
            break;
        case quxlang::cpu_tuning_model::x64_amd_zen2:
            result.tune_cpu = "znver2";
            break;
        case quxlang::cpu_tuning_model::x64_amd_zen3:
            result.tune_cpu = "znver3";
            break;
        case quxlang::cpu_tuning_model::x64_amd_zen4:
            result.tune_cpu = "znver4";
            break;
        case quxlang::cpu_tuning_model::x64_amd_zen5:
            result.tune_cpu = "znver5";
            break;
        case quxlang::cpu_tuning_model::x64_intel_haswell:
            result.tune_cpu = "haswell";
            break;
        case quxlang::cpu_tuning_model::x64_intel_skylake:
            result.tune_cpu = "skylake";
            break;
        case quxlang::cpu_tuning_model::x64_intel_skylake_avx512:
            result.tune_cpu = "skylake-avx512";
            break;
        case quxlang::cpu_tuning_model::x64_intel_icelake_client:
            result.tune_cpu = "icelake-client";
            break;
        case quxlang::cpu_tuning_model::x64_intel_icelake_server:
            result.tune_cpu = "icelake-server";
            break;
        case quxlang::cpu_tuning_model::x64_intel_alderlake:
            result.tune_cpu = "alderlake";
            break;
        case quxlang::cpu_tuning_model::x64_intel_sapphire_rapids:
            result.tune_cpu = "sapphirerapids";
            break;
        case quxlang::cpu_tuning_model::x64_intel_granite_rapids:
            result.tune_cpu = "graniterapids";
            break;
        case quxlang::cpu_tuning_model::arm_apple_m1:
            result.tune_cpu = "apple-m1";
            break;
        case quxlang::cpu_tuning_model::arm_apple_m2:
            result.tune_cpu = "apple-m2";
            break;
        case quxlang::cpu_tuning_model::arm_apple_m4:
            result.tune_cpu = "apple-m4";
            break;
        case quxlang::cpu_tuning_model::arm_apple_m5:
            result.tune_cpu = "apple-m5";
            break;
        }
    }

    /** Returns the default LLVM feature spelling for one canonical attribute token. */
    auto default_llvm_feature_name = [](std::string_view attribute) -> std::string
    {
        std::size_t separator = attribute.find('_');
        if (separator == std::string_view::npos || separator + 1 == attribute.size())
        {
            throw quxlang::compiler_bug("Malformed canonical CPU attribute " + std::string(attribute));
        }

        std::string result(attribute.substr(separator + 1));
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char character) -> char
        {
            if (character == '_')
            {
                return '-';
            }
            return static_cast< char >(std::tolower(character));
        });
        return result;
    };

    /** Returns explicit LLVM spellings which differ from the registry's default conversion. */
    auto explicit_llvm_feature_name = [](quxlang::cpu cpu_type, std::string_view attribute) -> std::optional< std::string_view >
    {
        using mapping = std::pair< std::string_view, std::string_view >;
        static std::map< quxlang::cpu, std::map< std::string_view, std::string_view > > mappings{
            {
                // Shared by X86 and X64; the X64-specific table below only adds its extensions.
                quxlang::cpu::x86_32,
                {
                    mapping{"FEATURE_AVX10_1", "avx10.1"}, mapping{"FEATURE_AVX10_1_512", "avx10.1-512"}, mapping{"FEATURE_AVX10_2", "avx10.2"}, mapping{"FEATURE_AVX10_2_512", "avx10.2-512"}, mapping{"FEATURE_BMI1", "bmi"}, mapping{"FEATURE_CMPXCHG8B", "cx8"}, mapping{"FEATURE_FXSAVE_FXRSTOR", "fxsr"}, mapping{"FEATURE_PCLMULQDQ", "pclmul"}, mapping{"FEATURE_PREFETCHW", "prfchw"}, mapping{"FEATURE_RDRAND", "rdrnd"}, mapping{"FEATURE_SSE4_1", "sse4.1"}, mapping{"FEATURE_SSE4_2", "sse4.2"}, mapping{"FEATURE_UNALIGNED_SSE_MEMORY", "sse-unaligned-mem"}, mapping{"PERF_BRANCH_FUSION", "branchfusion"}, mapping{"PERF_MACRO_FUSION", "macrofusion"}, mapping{"PERF_ENHANCED_REP_MOVSB_STOSB", "ermsb"}, mapping{"PERF_FAST_SHORT_REP_MOVSB", "fsrm"}, mapping{"PERF_FALSE_DEPENDENCY_GETMANT", "false-deps-getmant"}, mapping{"PERF_FALSE_DEPENDENCY_LZCNT_TZCNT", "false-deps-lzcnt-tzcnt"}, mapping{"PERF_FALSE_DEPENDENCY_MULC", "false-deps-mulc"}, mapping{"PERF_FALSE_DEPENDENCY_MULLQ", "false-deps-mullq"}, mapping{"PERF_FALSE_DEPENDENCY_PERMUTE", "false-deps-perm"}, mapping{"PERF_FALSE_DEPENDENCY_POPCNT", "false-deps-popcnt"}, mapping{"PERF_FALSE_DEPENDENCY_RANGE", "false-deps-range"}, mapping{"PERF_FAST_7_BYTE_NOP", "fast-7bytenop"}, mapping{"PERF_FAST_11_BYTE_NOP", "fast-11bytenop"}, mapping{"PERF_FAST_15_BYTE_NOP", "fast-15bytenop"}, mapping{"PERF_FAST_HORIZONTAL_OPS", "fast-hops"}, mapping{"PERF_FAST_IMMEDIATE_16", "fast-imm16"}, mapping{"PERF_FAST_IMMEDIATE_VECTOR_SHIFT", "tuning-fast-imm-vector-shift"}, mapping{"PERF_FAST_SCALAR_SQRT", "fast-scalar-fsqrt"}, mapping{"PERF_FAST_VECTOR_SQRT", "fast-vector-fsqrt"}, mapping{"PERF_FAST_SCALAR_SHIFT_MASK", "fast-scalar-shift-masks"}, mapping{"PERF_FAST_VECTOR_SHIFT_MASK", "fast-vector-shift-masks"}, mapping{"PERF_FAST_VARIABLE_CROSS_LANE_SHUFFLE", "fast-variable-crosslane-shuffle"}, mapping{"PERF_FAST_VARIABLE_PER_LANE_SHUFFLE", "fast-variable-perlane-shuffle"}, mapping{"PERF_SHIFT_FASTER_THAN_SHUFFLE", "faster-shift-than-shuffle"}, mapping{"PERF_LEA_USES_ADDRESS_GENERATION", "lea-uses-ag"}, mapping{"PERF_NO_BYPASS_DELAY_MOVE", "no-bypass-delay-mov"}, mapping{"PERF_SBB_DEPENDENCY_BREAKING", "sbb-dep-breaking"}, mapping{"PERF_SLOW_3_OPERAND_LEA", "slow-3ops-lea"}, mapping{"PERF_SLOW_INC_DEC", "slow-incdec"}, mapping{"PERF_SLOW_TWO_MEMORY_OPERANDS", "slow-two-mem-ops"}, mapping{"PERF_SLOW_UNALIGNED_MEMORY_16", "slow-unaligned-mem-16"}, mapping{"PERF_SLOW_UNALIGNED_MEMORY_32", "slow-unaligned-mem-32"},
                },
            },
            {
                quxlang::cpu::x86_64,
                {
                    mapping{"FEATURE_CMPXCHG16B", "cx16"},
                    mapping{"FEATURE_LAHF_SAHF", "sahf"},
                    mapping{"FEATURE_CET_SHADOW_STACK", "shstk"},
                    mapping{"FEATURE_APX_EGPR", "egpr"},
                    mapping{"FEATURE_APX_NDD", "ndd"},
                    mapping{"FEATURE_APX_NF", "nf"},
                    mapping{"FEATURE_APX_PPX", "ppx"},
                    mapping{"FEATURE_APX_PUSH2_POP2", "push2pop2"},
                    mapping{"FEATURE_APX_ZERO_UPPER", "zu"},
                    mapping{"FEATURE_KEY_LOCKER", "kl"},
                    mapping{"FEATURE_KEY_LOCKER_WIDE", "widekl"},
                },
            },
            {
                quxlang::cpu::arm_32,
                {
                    mapping{"FEATURE_V4", "armv4"}, mapping{"FEATURE_V4T", "armv4t"}, mapping{"FEATURE_V5T", "armv5t"}, mapping{"FEATURE_V5TE", "armv5te"}, mapping{"FEATURE_V5TEJ", "armv5tej"}, mapping{"FEATURE_V6", "armv6"}, mapping{"FEATURE_V6J", "armv6j"}, mapping{"FEATURE_V6K", "armv6k"}, mapping{"FEATURE_V6KZ", "armv6kz"}, mapping{"FEATURE_V6M", "armv6-m"}, mapping{"FEATURE_V6S_M", "armv6s-m"}, mapping{"FEATURE_V6T2", "armv6t2"}, mapping{"FEATURE_V7A", "armv7-a"}, mapping{"FEATURE_V7M", "armv7-m"}, mapping{"FEATURE_V7R", "armv7-r"}, mapping{"FEATURE_V7EM", "armv7e-m"}, mapping{"FEATURE_V7K", "armv7k"}, mapping{"FEATURE_V7S", "armv7s"}, mapping{"FEATURE_V7VE", "armv7ve"}, mapping{"FEATURE_V8A", "armv8-a"}, mapping{"FEATURE_V8M_BASE", "armv8-m.base"}, mapping{"FEATURE_V8M_MAIN", "armv8-m.main"}, mapping{"FEATURE_V8R", "armv8-r"}, mapping{"FEATURE_V8_1A", "armv8.1-a"}, mapping{"FEATURE_V8_1M_MAIN", "armv8.1-m.main"}, mapping{"FEATURE_V8_2A", "armv8.2-a"}, mapping{"FEATURE_V8_3A", "armv8.3-a"}, mapping{"FEATURE_V8_4A", "armv8.4-a"}, mapping{"FEATURE_V8_5A", "armv8.5-a"}, mapping{"FEATURE_V8_6A", "armv8.6-a"}, mapping{"FEATURE_V8_7A", "armv8.7-a"}, mapping{"FEATURE_V8_8A", "armv8.8-a"}, mapping{"FEATURE_V8_9A", "armv8.9-a"}, mapping{"FEATURE_V9A", "armv9-a"}, mapping{"FEATURE_V9_1A", "armv9.1-a"}, mapping{"FEATURE_V9_2A", "armv9.2-a"}, mapping{"FEATURE_V9_3A", "armv9.3-a"}, mapping{"FEATURE_V9_4A", "armv9.4-a"}, mapping{"FEATURE_V9_5A", "armv9.5-a"}, mapping{"FEATURE_V9_6A", "armv9.6-a"}, mapping{"FEATURE_V9_7A", "armv9.7-a"}, mapping{"FEATURE_A_PROFILE", "aclass"}, mapping{"FEATURE_R_PROFILE", "rclass"}, mapping{"FEATURE_M_PROFILE", "mclass"}, mapping{"FEATURE_DATA_BARRIER", "db"}, mapping{"FEATURE_FULL_DATA_BARRIER", "dfb"}, mapping{"FEATURE_HARDWARE_DIVIDE_THUMB", "hwdiv"}, mapping{"FEATURE_HARDWARE_DIVIDE_ARM", "hwdiv-arm"}, mapping{"FEATURE_V7_CLREX", "v7clrex"}, mapping{"FEATURE_V8M_SECURITY", "8msecext"}, mapping{"FEATURE_FP", "fp-armv8"}, mapping{"FEATURE_FP_ARMV8_D16", "fp-armv8d16"}, mapping{"FEATURE_FP_ARMV8_D16_SP", "fp-armv8d16sp"}, mapping{"FEATURE_FP_ARMV8_SP", "fp-armv8sp"}, mapping{"FEATURE_FP16", "fullfp16"}, mapping{"FEATURE_FP16_CONVERSION", "fp16"}, mapping{"FEATURE_FP_REGISTERS", "fpregs"}, mapping{"FEATURE_FP_REGISTERS_16", "fpregs16"}, mapping{"FEATURE_FP_REGISTERS_64", "fpregs64"}, mapping{"FEATURE_VFP2_SP", "vfp2sp"}, mapping{"FEATURE_VFP3_D16", "vfp3d16"}, mapping{"FEATURE_VFP3_D16_SP", "vfp3d16sp"}, mapping{"FEATURE_VFP3_SP", "vfp3sp"}, mapping{"FEATURE_VFP4_D16", "vfp4d16"}, mapping{"FEATURE_VFP4_D16_SP", "vfp4d16sp"}, mapping{"FEATURE_VFP4_SP", "vfp4sp"}, mapping{"FEATURE_MVE_FP", "mve.fp"}, mapping{"FEATURE_CRC32", "crc"}, mapping{"FEATURE_CDE_CP0", "cdecp0"}, mapping{"FEATURE_CDE_CP1", "cdecp1"}, mapping{"FEATURE_CDE_CP2", "cdecp2"}, mapping{"FEATURE_CDE_CP3", "cdecp3"}, mapping{"FEATURE_CDE_CP4", "cdecp4"}, mapping{"FEATURE_CDE_CP5", "cdecp5"}, mapping{"FEATURE_CDE_CP6", "cdecp6"}, mapping{"FEATURE_CDE_CP7", "cdecp7"}, mapping{"FEATURE_PMUV3", "perfmon"}, mapping{"PERF_MOVS_SHIFTED_OPERAND_EXPENSIVE", "avoid-movs-shop"}, mapping{"PERF_MULS_EXPENSIVE", "avoid-muls"}, mapping{"PERF_PARTIAL_CPSR_UPDATE_EXPENSIVE", "avoid-partial-cpsr"}, mapping{"PERF_MUXED_AGU_NEON_FPU", "muxed-units"}, mapping{"PERF_MVE_1_BEAT", "mve1beat"}, mapping{"PERF_MVE_2_BEAT", "mve2beat"}, mapping{"PERF_MVE_4_BEAT", "mve4beat"}, mapping{"PERF_RETURN_ADDRESS_STACK", "ret-addr-stack"}, mapping{"PERF_ZERO_CYCLE_ZEROING", "zcz"}, mapping{"PERF_SLOW_FP_COMPARE_BRANCH", "slow-fp-brcc"}, mapping{"PERF_SLOW_LOAD_D_SUBREGISTER", "slow-load-D-subreg"}, mapping{"PERF_SLOW_ODD_REGISTER", "slow-odd-reg"}, mapping{"PERF_SLOW_FP_FMA", "slowfpvfmx"}, mapping{"PERF_SLOW_FP_MAC", "slowfpvmlx"},
                },
            },
            {
                quxlang::cpu::arm_64,
                {
                    mapping{"FEATURE_V8_1A", "v8.1a"}, mapping{"FEATURE_V8_2A", "v8.2a"}, mapping{"FEATURE_V8_3A", "v8.3a"}, mapping{"FEATURE_V8_4A", "v8.4a"}, mapping{"FEATURE_V8_5A", "v8.5a"}, mapping{"FEATURE_V8_6A", "v8.6a"}, mapping{"FEATURE_V8_7A", "v8.7a"}, mapping{"FEATURE_V8_8A", "v8.8a"}, mapping{"FEATURE_V8_9A", "v8.9a"}, mapping{"FEATURE_V9_1A", "v9.1a"}, mapping{"FEATURE_V9_2A", "v9.2a"}, mapping{"FEATURE_V9_3A", "v9.3a"}, mapping{"FEATURE_V9_4A", "v9.4a"}, mapping{"FEATURE_V9_5A", "v9.5a"}, mapping{"FEATURE_V9_6A", "v9.6a"}, mapping{"FEATURE_V9_7A", "v9.7a"}, mapping{"FEATURE_FP", "fp-armv8"}, mapping{"FEATURE_ADVANCED_SIMD", "neon"}, mapping{"FEATURE_CRC32", "crc"}, mapping{"FEATURE_PMUV3", "perfmon"}, mapping{"FEATURE_FP16", "fullfp16"}, mapping{"FEATURE_FHM", "fp16fml"}, mapping{"FEATURE_FCMA", "complxnum"}, mapping{"FEATURE_JSCVT", "jsconv"}, mapping{"FEATURE_FRINTTS", "fptoint"}, mapping{"FEATURE_LRCPC", "rcpc"}, mapping{"FEATURE_LRCPC2", "rcpc-immo"}, mapping{"FEATURE_LRCPC3", "rcpc3"}, mapping{"FEATURE_CSV2_2", "specrestrict"}, mapping{"FEATURE_PAN2", "pan-rwv"}, mapping{"FEATURE_VHE", "vh"}, mapping{"FEATURE_CONTEXTIDR_EL2", "CONTEXTIDREL2"}, mapping{"FEATURE_SPEV1P2", "spe-eef"}, mapping{"FEATURE_UAO", "uaops"}, mapping{"FEATURE_DPB", "ccpp"}, mapping{"FEATURE_DPB2", "ccdp"}, mapping{"FEATURE_TRF", "tracev8.4"}, mapping{"FEATURE_AMUV1", "am"}, mapping{"FEATURE_AMUV1P1", "amvs"}, mapping{"FEATURE_TLBIOS_TLBIRANGE", "tlb-rmi"}, mapping{"FEATURE_FLAGM2", "altnzcv"}, mapping{"FEATURE_SPECRES", "predres"}, mapping{"FEATURE_RNG", "rand"}, mapping{"FEATURE_PRFMSLC", "prfm-slc-target"}, mapping{"FEATURE_S1POE2", "poe2"}, mapping{"PERF_ARITHMETIC_BCC_FUSION", "arith-bcc-fusion"}, mapping{"PERF_ARITHMETIC_CBZ_FUSION", "arith-cbz-fusion"}, mapping{"PERF_SLOW_ADDRESS_LSL_1_4", "addr-lsl-slow-14"}, mapping{"PERF_FAST_ALU_LSL_0_4", "alu-lsl-fast"}, mapping{"PERF_CHEAP_AS_MOVE_HANDLING", "exynos-cheap-as-move"}, mapping{"PERF_SELECT_EXPENSIVE", "predictable-select-expensive"}, mapping{"PERF_FUSE_ADDSUB_TWO_REGISTER_CONSTANT_ONE", "fuse-addsub-2reg-const1"}, mapping{"PERF_FUSE_ARITHMETIC_LOGIC", "fuse-arith-logic"}, mapping{"PERF_SLOW_MISALIGNED_128_STORE", "slow-misaligned-128store"}, mapping{"PERF_SLOW_STRQ_REGISTER_OFFSET_STORE", "slow-strqro-store"}, mapping{"PERF_ZERO_CYCLE_MOVE_FPR128", "zcm-fpr128"}, mapping{"PERF_ZERO_CYCLE_MOVE_FPR64", "zcm-fpr64"}, mapping{"PERF_ZERO_CYCLE_MOVE_FPR32", "zcm-fpr32"}, mapping{"PERF_ZERO_CYCLE_MOVE_GPR64", "zcm-gpr64"}, mapping{"PERF_ZERO_CYCLE_MOVE_GPR32", "zcm-gpr32"}, mapping{"PERF_ZERO_CYCLE_ZEROING_FPR128", "zcz-fpr128"}, mapping{"PERF_ZERO_CYCLE_ZEROING_GPR64", "zcz-gpr64"}, mapping{"PERF_ZERO_CYCLE_ZEROING_GPR32", "zcz-gpr32"},
                },
            },
            {
                // Shared by RISCV32 and RISCV64.
                quxlang::cpu::riscv_32,
                {
                    mapping{"PERF_BITFIELD_EXTRACT_FUSION", "bfext-fusion"},
                    mapping{"PERF_CONDITIONAL_CMOV_FUSION", "conditional-cmv-fusion"},
                    mapping{"PERF_DLEN_HALF_VLEN", "dlen-factor-2"},
                    mapping{"PERF_LOGARITHMIC_VRGATHER_LATENCY", "log-vrgather"},
                    mapping{"PERF_SELECT_EXPENSIVE", "predictable-select-expensive"},
                    mapping{"PERF_SINGLE_ELEMENT_VECTOR_FP64", "single-element-vec-fp64"},
                    mapping{"PERF_VECTOR_LENGTH_DEPENDENT_LATENCY", "vl-dependent-latency"},
                    mapping{"PERF_VXRM_WRITE_PIPELINE_FLUSH", "vxrm-pipeline-flush"},
                    mapping{"PERF_FAST_UNALIGNED_SCALAR_MEMORY", "unaligned-scalar-mem"},
                    mapping{"PERF_FAST_UNALIGNED_VECTOR_MEMORY", "unaligned-vector-mem"},
                },
            },
            {
                quxlang::cpu::z_arch,
                {
                    mapping{"FEATURE_DISTINCT_OPERANDS", "distinct-ops"},
                    mapping{"FEATURE_FLOATING_POINT_EXTENSION", "fp-extension"},
                    mapping{"FEATURE_INTERLOCKED_ACCESS_1", "interlocked-access1"},
                    mapping{"FEATURE_LOAD_STORE_ON_CONDITION", "load-store-on-cond"},
                    mapping{"FEATURE_LOAD_STORE_ON_CONDITION_2", "load-store-on-cond-2"},
                    mapping{"FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3", "message-security-assist-extension3"},
                    mapping{"FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_4", "message-security-assist-extension4"},
                    mapping{"FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_5", "message-security-assist-extension5"},
                    mapping{"FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_7", "message-security-assist-extension7"},
                    mapping{"FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_8", "message-security-assist-extension8"},
                    mapping{"FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_9", "message-security-assist-extension9"},
                    mapping{"FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_12", "message-security-assist-extension12"},
                },
            },
        };

        auto find_mapping = [&](quxlang::cpu mapping_cpu) -> std::optional< std::string_view >
        {
            std::map< quxlang::cpu, std::map< std::string_view, std::string_view > >::const_iterator cpu_mappings = mappings.find(mapping_cpu);
            if (cpu_mappings == mappings.end())
            {
                return std::nullopt;
            }
            std::map< std::string_view, std::string_view >::const_iterator found = cpu_mappings->second.find(attribute);
            if (found == cpu_mappings->second.end())
            {
                return std::nullopt;
            }
            return found->second;
        };

        if (std::optional< std::string_view > exact = find_mapping(cpu_type); exact.has_value())
        {
            return exact;
        }
        if (cpu_type == quxlang::cpu::x86_64)
        {
            return find_mapping(quxlang::cpu::x86_32);
        }
        if (cpu_type == quxlang::cpu::riscv_64)
        {
            return find_mapping(quxlang::cpu::riscv_32);
        }
        return std::nullopt;
    };

    std::vector< std::string > feature_settings;
    for (std::pair< std::string const, bool > const& attribute_setting : result.fixed_cpu_attribute_values)
    {
        std::optional< std::pair< quxlang::cpu, std::string > > parsed = quxlang::parse_cpu_attribute_stem(attribute_setting.first);
        if (!parsed.has_value() || parsed->first != machine.cpu_type)
        {
            throw quxlang::semantic_compilation_error("CPU stepping attribute does not apply to the LLVM target: " + attribute_setting.first);
        }

        if (parsed->second.starts_with("VENDOR_"))
        {
            continue;
        }

        bool enabled = attribute_setting.second;
        std::string feature_name;
        if (machine.cpu_type == quxlang::cpu::arm_64 && parsed->second == "PERF_ZERO_CYCLE_ZEROING_FPR64")
        {
            feature_name = "no-zcz-fpr64";
            enabled = !enabled;
        }
        else if (std::optional< std::string_view > explicit_name = explicit_llvm_feature_name(machine.cpu_type, parsed->second); explicit_name.has_value())
        {
            feature_name = *explicit_name;
        }
        else
        {
            feature_name = default_llvm_feature_name(parsed->second);
        }
        feature_settings.push_back(std::string(enabled ? "+" : "-") + feature_name);
    }

    for (std::size_t index = 0; index < feature_settings.size(); ++index)
    {
        if (index != 0)
        {
            result.target_features += ',';
        }
        result.target_features += feature_settings.at(index);
    }
    return result;
}

auto quxlang::llvm_backend::llvm_backend::preoptimize(quxlang::llvm_backend::llvm_compilable_unit const& input) const -> quxlang::llvm_backend::llvm_preoptimized_unit
{
    detail::llvm_module_codegen codegen(input);
    return codegen.preoptimize();
}

auto quxlang::llvm_backend::llvm_backend::postoptimize(quxlang::llvm_backend::llvm_preoptimized_unit const& input) const -> quxlang::llvm_backend::llvm_postoptimized_unit
{
    quxlang::llvm_backend::llvm_postoptimized_unit result;
    result.source_filename = input.source_filename;
    result.target = input.target;
    if (input.target.optimization == quxlang::llvm_backend::optimization_level::debug)
    {
        result.bitcode = input.bitcode;
        result.llvm_ir_text = input.llvm_ir_text;
        return result;
    }

    llvm::LLVMContext context;
    std::unique_ptr< llvm::Module > source_module = detail::llvm_module_codegen::parse_module_bitcode(input.bitcode, context, "quxlang-preoptimize.bc");
    std::unique_ptr< llvm::TargetMachine > target_machine = detail::llvm_module_codegen::create_target_machine(input.target);
    source_module->setTargetTriple(llvm::Triple(quxlang::lookup_llvm_triple(input.target.machine)));
    source_module->setDataLayout(target_machine->createDataLayout());
    std::unique_ptr< llvm::Module > optimized_module = detail::llvm_module_codegen::optimize_module(*source_module, target_machine.get());
    result.bitcode = detail::llvm_module_codegen::module_bitcode(*optimized_module);
    result.llvm_ir_text = detail::llvm_module_codegen::module_ir_text(*optimized_module);
    result.source_filename = optimized_module->getSourceFileName();
    return result;
}

auto quxlang::llvm_backend::llvm_backend::post_codegen(quxlang::llvm_backend::llvm_postoptimized_unit const& input) const -> quxlang::llvm_backend::llvm_post_codegen_unit
{
    llvm::LLVMContext context;
    std::unique_ptr< llvm::Module > module = detail::llvm_module_codegen::parse_module_bitcode(input.bitcode, context, "quxlang-postoptimize.bc");
    quxlang::llvm_backend::llvm_post_codegen_unit result;
    result.object_file = detail::llvm_module_codegen::emit_module_object_file(*module, input.target);
    return result;
}

auto quxlang::llvm_backend::llvm_backend::assemble(quxlang::llvm_backend::llvm_compilation_target const& target, quxlang::asm_procedure const& procedure) const -> quxlang::llvm_backend::llvm_assembled_procedure
{
    auto assembly_text = [&]() -> std::string
    {
        if (procedure.architecture == "ARM32" || procedure.architecture == "ARM64")
        {
            return quxlang::convert_to_gnu_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name, target.machine.binary_type == quxlang::binary::elf);
        }
        if (procedure.architecture == "Z_ARCH")
        {
            return quxlang::convert_to_gnu_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name, true);
        }
        if (procedure.architecture == "X64" || procedure.architecture == "X86")
        {
            return quxlang::convert_to_x64_asm(procedure.instructions.begin(), procedure.instructions.end(), procedure.name, target.machine.binary_type == quxlang::binary::elf);
        }
        throw quxlang::semantic_compilation_error("Unsupported asm procedure architecture for LLVM lowering: " + procedure.architecture);
    }();

    switch (target.machine.cpu_type)
    {
    case quxlang::cpu::x86_32:
    case quxlang::cpu::x86_64: {
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
    case quxlang::cpu::arm_32: {
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
    case quxlang::cpu::arm_64: {
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
    case quxlang::cpu::riscv_64: {
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
    case quxlang::cpu::z_arch: {
        static bool const initialized = []() -> bool
        {
            ::LLVMInitializeSystemZTargetInfo();
            ::LLVMInitializeSystemZTarget();
            ::LLVMInitializeSystemZTargetMC();
            ::LLVMInitializeSystemZAsmParser();
            ::LLVMInitializeSystemZAsmPrinter();
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

    std::unique_ptr< ::llvm::MCStreamer > streamer(llvm_target->createMCObjectStreamer(triple, machine_context, std::move(asm_backend), std::move(object_writer), std::move(code_emitter), *subtarget_info));
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
