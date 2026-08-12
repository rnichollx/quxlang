// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_LLVM_BACKEND_TYPES_HPP
#define QUXLANG_LLVM_BACKEND_TYPES_HPP

#include "data/machine.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/struct_layout.hpp>
#include <quxlang/data/enum_flagset_info.hpp>
#include <quxlang/data/fusion_info.hpp>
#include <quxlang/data/fusion_layout.hpp>
#include <quxlang/data/target_configuration.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/data/class_placement_info.hpp>
#include <quxlang/vmir2/source_index.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include <rpnx/macros.hpp>
#include <rpnx/cow.hpp>

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

RPNX_ENUM(quxlang::llvm_backend, optimization_level, std::uint64_t, debug, release);
RPNX_ENUM(quxlang::llvm_backend, runtime_procedure, std::uint64_t, assert_fail, panic, initguard_try_acquire, initguard_complete, initguard_abort);
/** Selects whether compiler-owned unit-test objects are declared or defined by a compilation packet. */
RPNX_ENUM(quxlang::llvm_backend, unit_test_object_emission, std::uint64_t, external_declarations, definitions);
/** Selects whether the packet defines its root routine or only declares it for another object. */
RPNX_ENUM(quxlang::llvm_backend, root_routine_emission, std::uint64_t, definition, external_declaration);

namespace quxlang::llvm_backend
{
    /// Identifies one initialized runtime procedure instantiation needed by LLVM lowering.
    struct runtime_procedure_reference
    {
        runtime_procedure procedure;

        RPNX_MEMBER_METADATA(runtime_procedure_reference, procedure);
    };

    /// Runtime values passed to MODULE(RUNTIME)::ASSERT_FAIL for one failed ASSERT.
    struct runtime_assert_fail_call_arguments
    {
        std::string expr;
        std::size_t file = 0;
        std::size_t line = 0;
        std::size_t column = 0;
        std::optional< std::string > tag;

        RPNX_MEMBER_METADATA(runtime_assert_fail_call_arguments, expr, file, line, column, tag);
    };

    /** Runtime values passed to MODULE(RUNTIME)::PANIC for one VMIR panic terminator. */
    struct runtime_panic_call_arguments
    {
        std::string message;
        std::size_t file = 0;
        std::size_t line = 0;
        std::size_t column = 0;

        RPNX_MEMBER_METADATA(runtime_panic_call_arguments, message, file, line, column);
    };

    /// llvm_compilation_target represents the compilation information needed to compile something to llvm
    struct llvm_compilation_target
    {
        machine_target_info machine;
        optimization_level optimization = optimization_level::debug;
        /// LLVM processor name used for target-specific optimized compilation.
        std::string cpu_name = "generic";
        /// LLVM target-feature string used for target-specific optimized compilation.
        std::string target_features;
        /// Optional LLVM processor name used for scheduling and cost-model tuning only.
        std::optional< std::string > tune_cpu;

        RPNX_MEMBER_METADATA(llvm_compilation_target, machine, optimization, cpu_name, target_features, tune_cpu);
    };

    /** Verified LLVM module state before the configured optimization pipeline runs. */
    struct llvm_preoptimized_unit
    {
        std::vector<std::byte> bitcode;
        std::string llvm_ir_text;
        /// Compilation-root-relative filename selected for module-level source metadata.
        std::string source_filename;
        /// Target retained by every later stage of this component's compilation.
        llvm_compilation_target target;

        RPNX_MEMBER_METADATA(llvm_preoptimized_unit, bitcode, llvm_ir_text, source_filename, target);
    };

    /** LLVM module state after the configured optimization stage and before machine code generation. */
    struct llvm_postoptimized_unit
    {
        std::vector<std::byte> bitcode;
        std::string llvm_ir_text;
        std::string source_filename;
        llvm_compilation_target target;

        RPNX_MEMBER_METADATA(llvm_postoptimized_unit, bitcode, llvm_ir_text, source_filename, target);
    };

    /** Independently linkable native object emitted by the post-codegen stage. */
    struct llvm_post_codegen_unit
    {
        std::vector<std::byte> object_file;

        RPNX_MEMBER_METADATA(llvm_post_codegen_unit, object_file);
    };

    /// llvm_compiled_unit retains the observable results of all three LLVM compilation stages for one module.
    struct llvm_compiled_unit
    {
        std::vector<std::byte> bitcode;
        std::string llvm_ir_text;
        std::string postoptimized_llvm_ir_text;
        /// Compilation-root-relative filename selected for module-level source metadata.
        std::string source_filename;
        std::vector<std::byte> object_file;

        RPNX_MEMBER_METADATA(llvm_compiled_unit, bitcode, llvm_ir_text, postoptimized_llvm_ir_text, source_filename, object_file);
    };

    /// llvm_assembled_procedure represents the emitted text and object bytes for one machine-specific asm procedure.
    struct llvm_assembled_procedure
    {
        std::string assembly_text;
        std::vector<std::byte> object_file;

        RPNX_MEMBER_METADATA(llvm_assembled_procedure, assembly_text, object_file);
    };

    /// One UNIT_TEST entry emitted into a runtime unit-test suite table.
    struct unit_test_entry
    {
        std::string name;
        type_symbol procedure_symbol;

        RPNX_MEMBER_METADATA(unit_test_entry, name, procedure_symbol);
    };

    /** Inputs used to emit the compiler-generated CPU detection and stepping-selection interface. */
    struct cpu_stepping_support
    {
        std::vector< cpu_stepping_configuration > steppings;
        /// Concrete zero-argument runtime detector functanoid for each complete CPU attribute stem.
        std::map< std::string, type_symbol > attribute_detectors;

        RPNX_MEMBER_METADATA(cpu_stepping_support, steppings, attribute_detectors);
    };

    /// llvm_compilable_unit represents a unit of code for llvm to compile with all required input information
    struct llvm_compilable_unit
    {
        type_symbol target_name;
        vmir2::functanoid_routine3 target_code;
        llvm_compilation_target machine_target;
        /// Zero-based program stepping index to associate with this compilation.
        std::size_t stepping_index = 0;
        /// Places every function definition in the section owned by stepping_index.
        bool place_definitions_in_stepping_section = false;
        /// Appends the program stepping suffix to every LLVM-defined function symbol.
        bool suffix_generated_function_symbols = false;
        /// Allows identical definitions shared by independently compiled closures to be coalesced during linking.
        bool definitions_are_coalescible = false;
        /// Allows this unit to synthesize a platform process entrypoint when one is not supplied.
        bool emit_process_entrypoint = true;
        /// Selects whether this packet owns the root routine body.
        root_routine_emission root_routine = root_routine_emission::definition;
        /// Selects whether compiler-owned objects referenced by this packet are defined here.
        bool defines_compiler_builtin_objects = false;
        /// whole_module requests that indirect helper definitions stay in the emitted module as linkonce_odr bodies.
        bool whole_module = false;
        /// whole_module_output_kind describes the final artifact kind when this packet is one aggregate output module.
        std::optional< output_kind > whole_module_output_kind;
        /// executable_entry_symbol names an externally provided process entrypoint for executable output modules.
        std::optional< std::string > executable_entry_symbol;
        /// Concrete MODULE(RUNTIME)::POST_DETECT functanoid emitted in each stepped program module.
        std::optional< type_symbol > post_detect_functanoid;
        /// Unit tests exposed to MODULE(RUNTIME)::UNIT_TEST_MAIN in unit_test_suite outputs.
        std::vector< unit_test_entry > unit_tests;
        /// Controls ownership of UNIT_TEST_COUNT, UNIT_TEST_NAMES, and UNIT_TEST_PROC in this module.
        unit_test_object_emission unit_test_objects = unit_test_object_emission::external_declarations;
        /// Requests compiler-generated DETECT_CPU_ARCHINFO, PICK_STEPPING, and STEPPING_COUNT definitions.
        std::optional< cpu_stepping_support > stepping_support;
        std::optional< rpnx::cow< vmir2::source_index > > source_index;
        std::map<type_symbol, vmir2::functanoid_routine3> inlinable_functions;
        /// Selected callable ABI surfaces for asm procedures, keyed by the concrete called functanoid symbol.
        std::map<type_symbol, asm_callable> asm_callable_interfaces;
        /// Asm procedure bodies, keyed by the declaration symbol that owns the emitted machine-code label.
        std::map<type_symbol, asm_procedure> asm_functions;
        /// Runtime procedure instantiations needed by lowering, keyed by abstract runtime role.
        std::map<runtime_procedure_reference, type_symbol> runtime_procedures;
        std::map<type_symbol, std::string> procedure_linksymbols;
        std::set<type_symbol> extern_procedures;
        std::set<type_symbol> optional_extern_procedures;
        /// Logical external library names for procedures resolved by the platform loader.
        std::map<type_symbol, std::string> extern_procedure_libraries;
        std::map<type_symbol, std::string> extern_procedure_versions;
        std::map<type_symbol, type_symbol> object_reference_types;
        std::map<type_symbol, antestatal_value> antestatal_constants;
        /// Dense canonical type ordinals shared by every component of one linked output.
        std::map<type_symbol, std::uint64_t> type_index_ordinals;
        std::map<type_symbol, initialization_type> global_init_types;
        std::map<type_symbol, std::vector< interface_slot_key > > interface_slots;
        std::map<type_symbol, enum_info> enum_infos;
        std::map<type_symbol, flagset_info> flagset_infos;
        std::map<type_symbol, struct_layout> struct_layouts;
        std::map<type_symbol, union_info> union_infos;
        std::map<type_symbol, variant_info> variant_infos;
        std::map<type_symbol, fusion_layout> fusion_layouts;
        std::map<type_symbol, class_placement_info> type_placements;

        RPNX_MEMBER_METADATA(llvm_compilable_unit, target_name, target_code, machine_target, stepping_index, place_definitions_in_stepping_section, suffix_generated_function_symbols, definitions_are_coalescible, emit_process_entrypoint, root_routine, defines_compiler_builtin_objects, whole_module, whole_module_output_kind, executable_entry_symbol, post_detect_functanoid, unit_tests, unit_test_objects, stepping_support, source_index, inlinable_functions, asm_callable_interfaces, asm_functions, runtime_procedures, procedure_linksymbols, extern_procedures, optional_extern_procedures, extern_procedure_libraries, extern_procedure_versions, object_reference_types, antestatal_constants, type_index_ordinals, global_init_types, interface_slots, enum_infos, flagset_infos, struct_layouts, union_infos, variant_infos, fusion_layouts, type_placements);
    };

    /// Returns true when a type symbol names the requested builtin object.
    inline auto builtin_symbol_named(type_symbol const& symbol, std::string_view name) -> bool
    {
        return symbol.type_is< builtin_symbol >() && symbol.get_as< builtin_symbol >().name == name;
    }

    /// Returns true when symbol is the MAIN_FUNCTION_ARRAY builtin object.
    inline auto is_main_function_array_symbol(type_symbol const& symbol) -> bool
    {
        return builtin_symbol_named(symbol, "MAIN_FUNCTION_ARRAY");
    }

    /// Returns true when symbol is the POST_DETECT_FUNCTION_ARRAY builtin object.
    inline auto is_post_detect_function_array_symbol(type_symbol const& symbol) -> bool
    {
        return builtin_symbol_named(symbol, "POST_DETECT_FUNCTION_ARRAY");
    }

    /** Returns the type of MAIN_FUNCTION_ARRAY for a target stepping count. */
    inline auto main_function_array_object_type(std::size_t stepping_count) -> type_symbol
    {
        procedure_type procedure;
        procedure.signature.return_type = int_type{.bits = 32, .has_sign = true};
        return array_type{
            .element_type = std::move(procedure),
            .element_count = expression_numeric_literal{.value = std::to_string(stepping_count)},
        };
    }

    /** Returns the type of POST_DETECT_FUNCTION_ARRAY for a target stepping count. */
    inline auto post_detect_function_array_object_type(std::size_t stepping_count) -> type_symbol
    {
        procedure_type procedure;
        procedure.signature.return_type = void_type{};
        return array_type{
            .element_type = std::move(procedure),
            .element_count = expression_numeric_literal{.value = std::to_string(stepping_count)},
        };
    }

    /// Returns true when symbol is the ACTIVE_STEPPING builtin object.
    inline auto is_active_stepping_symbol(type_symbol const& symbol) -> bool
    {
        return builtin_symbol_named(symbol, "ACTIVE_STEPPING");
    }

    /// Returns true when symbol is the STEPPING_COUNT builtin object.
    inline auto is_stepping_count_symbol(type_symbol const& symbol) -> bool
    {
        return builtin_symbol_named(symbol, "STEPPING_COUNT");
    }

    /// Returns true when symbol is the UNIT_TEST_COUNT builtin object.
    inline auto is_unit_test_count_object_symbol(type_symbol const& symbol) -> bool
    {
        return builtin_symbol_named(symbol, "UNIT_TEST_COUNT");
    }

    /// Returns true when symbol is the UNIT_TEST_NAMES builtin object.
    inline auto is_unit_test_names_object_symbol(type_symbol const& symbol) -> bool
    {
        return builtin_symbol_named(symbol, "UNIT_TEST_NAMES");
    }

    /// Returns true when symbol is the UNIT_TEST_PROC builtin object.
    inline auto is_unit_test_proc_object_symbol(type_symbol const& symbol) -> bool
    {
        return builtin_symbol_named(symbol, "UNIT_TEST_PROC");
    }

    /// Returns the source-level runtime procedure symbol for one abstract runtime procedure.
    inline auto runtime_procedure_initializee(runtime_procedure procedure) -> type_symbol
    {
        switch (procedure)
        {
        case runtime_procedure::assert_fail:
            return subsymbol{
                .of = absolute_module_reference{.module_name = "RUNTIME"},
                .name = "ASSERT_FAIL",
            };
        case runtime_procedure::panic:
            return subsymbol{
                .of = absolute_module_reference{.module_name = "RUNTIME"},
                .name = "PANIC",
            };
        case runtime_procedure::initguard_try_acquire:
            return subsymbol{
                .of = absolute_module_reference{.module_name = "RUNTIME"},
                .name = "INITGUARD_TRY_ACQUIRE",
            };
        case runtime_procedure::initguard_complete:
            return subsymbol{
                .of = absolute_module_reference{.module_name = "RUNTIME"},
                .name = "INITGUARD_COMPLETE",
            };
        case runtime_procedure::initguard_abort:
            return subsymbol{
                .of = absolute_module_reference{.module_name = "RUNTIME"},
                .name = "INITGUARD_ABORT",
            };
        }
        throw compiler_bug("unknown runtime procedure");
    }

    /** Returns the source and linker symbol for the runtime stack-probe procedure. */
    inline auto runtime_check_stack_symbol() -> type_symbol
    {
        return subsymbol{
            .of = absolute_module_reference{.module_name = "RUNTIME"},
            .name = "CHECK_STACK",
        };
    }

    /// Returns the constant type used by runtime ASSERT_FAIL string parameters.
    inline auto runtime_string_constant_type() -> type_symbol
    {
        return readonly_constant{.kind = constant_kind::string};
    }

    /// Returns the object type of UNIT_TEST_NAMES.
    inline auto unit_test_names_object_type() -> type_symbol
    {
        return ptrref_type{
            .target = runtime_string_constant_type(),
            .ptr_class = pointer_class::array,
            .qual = qualifier::constant,
        };
    }

    /// Returns the object type of UNIT_TEST_PROC.
    inline auto unit_test_proc_object_type() -> type_symbol
    {
        return ptrref_type{
            .target = procedure_type{},
            .ptr_class = pointer_class::array,
            .qual = qualifier::constant,
        };
    }

    /// Returns true when symbol is one of the unit-test suite builtin objects.
    inline auto is_unit_test_object_symbol(type_symbol const& symbol) -> bool
    {
        return is_unit_test_count_object_symbol(symbol) || is_unit_test_names_object_symbol(symbol) || is_unit_test_proc_object_symbol(symbol);
    }

    /// Returns the constant pointer type used by runtime ASSERT_FAIL's tag parameter.
    inline auto runtime_string_constant_cptr_type() -> type_symbol
    {
        return ptrref_type{
            .target = runtime_string_constant_type(),
            .ptr_class = pointer_class::instance,
            .qual = qualifier::constant,
        };
    }

    /// Returns the fixed call signature used to initialize MODULE(RUNTIME)::ASSERT_FAIL.
    inline auto runtime_assert_fail_parameters() -> instatype
    {
        type_symbol const string_constant_type = runtime_string_constant_type();
        type_symbol const tag_type = runtime_string_constant_cptr_type();
        type_symbol const sz_type = size_type{};

        instatype parameters;
        parameters.named["expr"] = make_type_instantiation(string_constant_type);
        parameters.named["file"] = make_type_instantiation(sz_type);
        parameters.named["line"] = make_type_instantiation(sz_type);
        parameters.named["column"] = make_type_instantiation(sz_type);
        parameters.named["tag"] = make_type_instantiation(tag_type);
        return parameters;
    }

    /** Returns the fixed call signature used to initialize MODULE(RUNTIME)::PANIC. */
    inline auto runtime_panic_parameters() -> instatype
    {
        type_symbol const string_constant_type = runtime_string_constant_type();
        type_symbol const sz_type = size_type{};

        instatype parameters;
        parameters.named["message"] = make_type_instantiation(string_constant_type);
        parameters.named["file"] = make_type_instantiation(sz_type);
        parameters.named["line"] = make_type_instantiation(sz_type);
        parameters.named["column"] = make_type_instantiation(sz_type);
        return parameters;
    }

    /// Returns the fixed call signature used to initialize MODULE(RUNTIME)'s initguard procedures.
    inline auto runtime_initguard_parameters() -> instatype
    {
        instatype parameters;
        parameters.named["guard"] = make_type_instantiation(ptrref_type{
            .target = initguard_type{},
            .ptr_class = pointer_class::ref,
            .qual = qualifier::mut,
        });
        return parameters;
    }

    /// Returns the direct LLVM return slot type for one runtime procedure when it returns a value.
    inline auto runtime_procedure_return_type(runtime_procedure procedure) -> std::optional< type_symbol >
    {
        switch (procedure)
        {
        case runtime_procedure::assert_fail:
        case runtime_procedure::panic:
        case runtime_procedure::initguard_complete:
        case runtime_procedure::initguard_abort:
            return std::nullopt;
        case runtime_procedure::initguard_try_acquire:
            return bool_type{};
        }
        throw compiler_bug("unknown runtime procedure");
    }

    /// Returns the initialization request for one abstract runtime procedure.
    inline auto runtime_procedure_initialization(runtime_procedure procedure) -> initialization_reference
    {
        initialization_reference initialization{
            .initializee = runtime_procedure_initializee(procedure),
        };

        switch (procedure)
        {
        case runtime_procedure::assert_fail:
            initialization.parameters = runtime_assert_fail_parameters();
            return initialization;
        case runtime_procedure::panic:
            initialization.parameters = runtime_panic_parameters();
            return initialization;
        case runtime_procedure::initguard_try_acquire:
        case runtime_procedure::initguard_complete:
        case runtime_procedure::initguard_abort:
            initialization.parameters = runtime_initguard_parameters();
            return initialization;
        }
        throw compiler_bug("unknown runtime procedure");
    }

    /// Builds the runtime ASSERT_FAIL call argument payload for one VMIR ASSERT instruction.
    inline auto runtime_assert_fail_arguments(vmir2::assert_instr const& instruction, vmir2::source_index const& source_index) -> runtime_assert_fail_call_arguments
    {
        std::size_t file = 0;
        std::size_t line = 0;
        std::size_t column = 0;
        if (instruction.location.has_value())
        {
            file = instruction.location->file_id;
            auto const file_iter = source_index.files.find(instruction.location->file_id);
            if (file_iter != source_index.files.end())
            {
                vmir2::source_position const position = file_iter->second.position(instruction.location->begin_index);
                line = position.line;
                column = position.column;
            }
        }

        return runtime_assert_fail_call_arguments{
            .expr = instruction.expr_text,
            .file = file,
            .line = line,
            .column = column,
            .tag = instruction.tag,
        };
    }

    /** Builds the runtime PANIC call payload for one VMIR panic terminator. */
    inline auto runtime_panic_arguments(vmir2::panic const& terminator, vmir2::source_index const& source_index) -> runtime_panic_call_arguments
    {
        std::size_t file = 0;
        std::size_t line = 0;
        std::size_t column = 0;
        if (terminator.location.has_value())
        {
            file = terminator.location->file_id;
            std::map< std::uint64_t, vmir2::indexed_source_file >::const_iterator const file_iter = source_index.files.find(terminator.location->file_id);
            if (file_iter != source_index.files.end())
            {
                vmir2::source_position const position = file_iter->second.position(terminator.location->begin_index);
                line = position.line;
                column = position.column;
            }
        }

        return runtime_panic_call_arguments{
            .message = terminator.message,
            .file = file,
            .line = line,
            .column = column,
        };
    }


}

#endif // QUXLANG_LLVM_BACKEND_TYPES_HPP
