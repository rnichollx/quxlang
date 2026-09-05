// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_LLVM_BACKEND_TYPES_HPP
#define QUXLANG_LLVM_BACKEND_TYPES_HPP

#include "data/machine.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/class_placement_info.hpp>
#include <quxlang/data/dependencies.hpp>
#include <quxlang/data/enum_flagset_info.hpp>
#include <quxlang/data/fusion_info.hpp>
#include <quxlang/data/fusion_layout.hpp>
#include <quxlang/data/struct_layout.hpp>
#include <quxlang/data/target_configuration.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/vmir2/source_index.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include <rpnx/cow.hpp>
#include <rpnx/macros.hpp>

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

RPNX_ENUM(quxlang::llvm_backend, runtime_procedure, std::uint64_t, assert_fail, panic, initguard_try_acquire, thread_initguard_try_acquire, initguard_complete, initguard_abort, thread_destructor_register);
/** Selects whether compiler-owned unit-test objects are declared or defined by a compilation packet. */
RPNX_ENUM(quxlang::llvm_backend, unit_test_object_emission, std::uint64_t, external_declarations, definitions);
/** Selects whether the packet defines its root routine or only declares it for another object. */
RPNX_ENUM(quxlang::llvm_backend, root_routine_emission, std::uint64_t, definition, external_declaration);

namespace quxlang::llvm_backend
{
    /** Resolves a compiler-generated VMIR call to its LLVM runtime procedure. */
    inline auto runtime_procedure_from_dependency(vmir_runtime_dependency dependency) -> runtime_procedure
    {
        switch (dependency)
        {
        case vmir_runtime_dependency::assert_fail:
            return runtime_procedure::assert_fail;
        case vmir_runtime_dependency::panic:
            return runtime_procedure::panic;
        case vmir_runtime_dependency::initguard_complete:
            return runtime_procedure::initguard_complete;
        case vmir_runtime_dependency::initguard_abort:
            return runtime_procedure::initguard_abort;
        case vmir_runtime_dependency::initguard_try_acquire:
            return runtime_procedure::initguard_try_acquire;
        case vmir_runtime_dependency::thread_initguard_try_acquire:
            return runtime_procedure::thread_initguard_try_acquire;
        case vmir_runtime_dependency::thread_destructor_register:
            return runtime_procedure::thread_destructor_register;
        }
        throw compiler_bug("Invalid VMIR runtime dependency");
    }

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
        quxlang::build_type build_type = quxlang::build_type::debug;
        /// LLVM processor name used for target-specific optimized compilation.
        std::string cpu_name = "generic";
        /// LLVM target-feature string used for target-specific optimized compilation.
        std::string target_features;
        /// Optional LLVM processor name used for scheduling and cost-model tuning only.
        std::optional< std::string > tune_cpu;
        /// Individual stable CPU attributes whose runtime values are fixed in this stepping.
        std::map< std::string, bool > fixed_cpu_attribute_values;

        RPNX_MEMBER_METADATA(llvm_compilation_target, machine, build_type, cpu_name, target_features, tune_cpu, fixed_cpu_attribute_values);
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

    /** Indexes immutable compiler metadata without copying its payload. */
    template < typename Value >
    using llvm_borrowed_type_map = std::map< type_symbol, std::reference_wrapper< Value const > >;

    /** Synchronous LLVM input view; all borrowed payloads must outlive backend generation. */
    struct llvm_compilable_unit
    {
        /// Shared unoptimized routine catalog used for lazy declarations in partitioned modules.
        std::map< type_symbol, vmir2::functanoid_routine3 const* > procedure_declarations;
        /// True for support and procedure modules of a partitioned component.
        bool partitioned = false;
        /// True only for the module owning component data and assembly.
        bool owns_support_data = true;
        /// Output-wide construction-phase table capacity shared by every compilation unit.
        std::size_t struct_phase_assignment_capacity = 1;
        /// Disambiguates component-owned descriptor symbols in partitioned outputs.
        std::string support_symbol_suffix;
        /// Procedures referenced by module assembly must retain their external symbol identity.
        std::set< type_symbol > assembly_referenced_procedures;
        type_symbol target_name;
        vmir2::functanoid_routine3 const* target_code = nullptr;
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
        /// Selects complete component visibility for optimization.
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
        /** Borrows source records for the duration of synchronous backend generation. */
        vmir2::source_index const* source_index = nullptr;
        llvm_borrowed_type_map< vmir2::functanoid_routine3 > inlinable_functions;
        /// Selected callable ABI surfaces for asm procedures, keyed by the concrete called functanoid symbol.
        llvm_borrowed_type_map< asm_callable > asm_callable_interfaces;
        /// Asm procedure bodies, keyed by the declaration symbol that owns the emitted machine-code label.
        llvm_borrowed_type_map< asm_procedure > asm_functions;
        /// Runtime procedure instantiations needed by lowering, keyed by abstract runtime role.
        std::map<runtime_procedure_reference, type_symbol> runtime_procedures;
        std::map<type_symbol, std::string> procedure_linksymbols;
        std::set<type_symbol> extern_procedures;
        std::set<type_symbol> optional_extern_procedures;
        /// Logical external library names for procedures resolved by the platform loader.
        std::map<type_symbol, std::string> extern_procedure_libraries;
        std::map<type_symbol, std::string> extern_procedure_versions;
        std::map<type_symbol, type_symbol> object_reference_types;
        llvm_borrowed_type_map< antestatal_value > antestatal_constants;
        /// Dense canonical type ordinals shared by every component of one linked output.
        rpnx::cow< std::map< type_symbol, std::uint64_t > > type_index_ordinals;
        std::map<type_symbol, initialization_type> global_init_types;
        llvm_borrowed_type_map< std::vector< interface_slot > > interface_slots;
        llvm_borrowed_type_map< enum_info > enum_infos;
        llvm_borrowed_type_map< flagset_info > flagset_infos;
        llvm_borrowed_type_map< struct_layout > struct_layouts;
        llvm_borrowed_type_map< struct_runtime_info > struct_runtime_infos;
        llvm_borrowed_type_map< union_info > union_infos;
        llvm_borrowed_type_map< variant_info > variant_infos;
        llvm_borrowed_type_map< fusion_layout > fusion_layouts;
        llvm_borrowed_type_map< class_placement_info > type_placements;
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

    /** Returns the count-independent constant array pointer exposed by the dispatch builtin. */
    inline auto main_function_array_object_type() -> type_symbol
    {
        return ptrref_type{
            .target = procedure_type{.signature = sigtype{.return_type = int_type{.bits = 32, .has_sign = true}}},
            .ptr_class = pointer_class::array,
            .qual = qualifier::constant,
        };
    }

    /** Returns the count-independent constant array pointer exposed by the dispatch builtin. */
    inline auto post_detect_function_array_object_type() -> type_symbol
    {
        return ptrref_type{
            .target = procedure_type{.signature = sigtype{.return_type = void_type{}}},
            .ptr_class = pointer_class::array,
            .qual = qualifier::constant,
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
        case runtime_procedure::thread_initguard_try_acquire:
            return subsymbol{
                .of = absolute_module_reference{.module_name = "RUNTIME"},
                .name = "THREAD_INITGUARD_TRY_ACQUIRE",
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
        case runtime_procedure::thread_destructor_register:
            return subsymbol{
                .of = absolute_module_reference{.module_name = "RUNTIME"},
                .name = "THREAD_DESTRUCTOR_REGISTER",
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
            .target = procedure_type{.signature = sigtype{.return_type = void_type{}}},
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
    inline auto runtime_assert_fail_parameters(type_symbol const& uintpointer_type) -> instatype
    {
        type_symbol const string_constant_type = runtime_string_constant_type();
        type_symbol const tag_type = runtime_string_constant_cptr_type();

        instatype parameters;
        parameters.named["expr"] = make_type_instantiation(string_constant_type);
        parameters.named["file"] = make_type_instantiation(uintpointer_type);
        parameters.named["line"] = make_type_instantiation(uintpointer_type);
        parameters.named["column"] = make_type_instantiation(uintpointer_type);
        parameters.named["tag"] = make_type_instantiation(tag_type);
        return parameters;
    }

    /** Returns the fixed call signature used to initialize MODULE(RUNTIME)::PANIC. */
    inline auto runtime_panic_parameters(type_symbol const& uintpointer_type) -> instatype
    {
        type_symbol const string_constant_type = runtime_string_constant_type();

        instatype parameters;
        parameters.named["message"] = make_type_instantiation(string_constant_type);
        parameters.named["file"] = make_type_instantiation(uintpointer_type);
        parameters.named["line"] = make_type_instantiation(uintpointer_type);
        parameters.named["column"] = make_type_instantiation(uintpointer_type);
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

    /** Returns the runtime-owned node type used by thread-local destructor registration. */
    inline auto runtime_thread_destructor_node_type() -> type_symbol
    {
        return subsymbol{
            .of = absolute_module_reference{.module_name = "RUNTIME"},
            .name = "thread_destructor_node",
        };
    }

    /** Returns the fixed call signature used to register one thread-local destructor node. */
    inline auto runtime_thread_destructor_register_parameters() -> instatype
    {
        instatype parameters;
        parameters.named["node"] = make_type_instantiation(ptrref_type{
            .target = runtime_thread_destructor_node_type(),
            .ptr_class = pointer_class::ref,
            .qual = qualifier::mut,
        });
        parameters.named["guard"] = make_type_instantiation(ptrref_type{
            .target = initguard_type{},
            .ptr_class = pointer_class::ref,
            .qual = qualifier::mut,
        });
        parameters.named["deinitializer"] = make_type_instantiation(ptrref_type{
            .target = procedure_type{.signature = sigtype{.return_type = void_type{}}},
            .ptr_class = pointer_class::instance,
            .qual = qualifier::constant,
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
        case runtime_procedure::thread_destructor_register:
            return std::nullopt;
        case runtime_procedure::initguard_try_acquire:
        case runtime_procedure::thread_initguard_try_acquire:
            return bool_type{};
        }
        throw compiler_bug("unknown runtime procedure");
    }

    /// Returns the initialization request for one abstract runtime procedure using a canonical target pointer-sized type.
    inline auto runtime_procedure_initialization(runtime_procedure procedure, type_symbol const& uintpointer_type) -> initialization_reference
    {
        initialization_reference initialization{
            .initializee = runtime_procedure_initializee(procedure),
        };

        switch (procedure)
        {
        case runtime_procedure::assert_fail:
            initialization.parameters = runtime_assert_fail_parameters(uintpointer_type);
            return initialization;
        case runtime_procedure::panic:
            initialization.parameters = runtime_panic_parameters(uintpointer_type);
            return initialization;
        case runtime_procedure::initguard_try_acquire:
        case runtime_procedure::thread_initguard_try_acquire:
        case runtime_procedure::initguard_complete:
        case runtime_procedure::initguard_abort:
            initialization.parameters = runtime_initguard_parameters();
            return initialization;
        case runtime_procedure::thread_destructor_register:
            initialization.parameters = runtime_thread_destructor_register_parameters();
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
