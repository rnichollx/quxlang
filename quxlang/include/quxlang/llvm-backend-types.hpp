// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_LLVM_BACKEND_TYPES_HPP
#define QUXLANG_LLVM_BACKEND_TYPES_HPP

#include "data/machine.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/class_layout.hpp>
#include <quxlang/data/enum_flagset_info.hpp>
#include <quxlang/data/target_configuration.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/data/type_placement_info.hpp>
#include <quxlang/vmir2/source_index.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include <rpnx/macros.hpp>
#include <rpnx/cow.hpp>

RPNX_ENUM(quxlang::llvm_backend, optimization_level, std::uint64_t, debug, release);

namespace quxlang::llvm_backend
{
    /// llvm_compiled_unit represents the results of compiling some functanoid to LLVM
    struct llvm_compiled_unit
    {
        std::vector<std::byte> bitcode;
        std::string llvm_ir_text;
        std::string optimized_llvm_ir_text;
        std::vector<std::byte> object_file;
        std::vector<std::byte> optimized_object_file;

        RPNX_MEMBER_METADATA(llvm_compiled_unit, bitcode, llvm_ir_text, optimized_llvm_ir_text, object_file, optimized_object_file);
    };

    /// llvm_assembled_procedure represents the emitted text and object bytes for one machine-specific asm procedure.
    struct llvm_assembled_procedure
    {
        std::string assembly_text;
        std::vector<std::byte> object_file;

        RPNX_MEMBER_METADATA(llvm_assembled_procedure, assembly_text, object_file);
    };

    /// llvm_compilation_target represents the compilation information needed to compile something to llvm
    struct llvm_compilation_target
    {
        machine_target_info machine;
        optimization_level optimization = optimization_level::debug;

        RPNX_MEMBER_METADATA(llvm_compilation_target, machine, optimization);
    };

    /// llvm_compilable_unit represents a unit of code for llvm to compile with all required input information
    struct llvm_compilable_unit
    {
        type_symbol target_name;
        vmir2::functanoid_routine3 target_code;
        llvm_compilation_target machine_target;
        /// whole_module requests that indirect helper definitions stay in the emitted module as linkonce_odr bodies.
        bool whole_module = false;
        /// whole_module_output_kind describes the final artifact kind when this packet is one aggregate output module.
        std::optional< output_kind > whole_module_output_kind;
        /// executable_entry_symbol names an externally provided process entrypoint for executable output modules.
        std::optional< std::string > executable_entry_symbol;
        std::optional< rpnx::cow< vmir2::source_index > > source_index;
        std::map<type_symbol, vmir2::functanoid_routine3> inlinable_functions;
        std::map<type_symbol, asm_procedure> asm_functions;
        std::map<type_symbol, std::string> procedure_linksymbols;
        std::map<type_symbol, antestatal_value> antestatal_constants;
        std::map<type_symbol, std::vector< interface_slot_key > > interface_slots;
        std::map<type_symbol, enum_info> enum_infos;
        std::map<type_symbol, flagset_info> flagset_infos;
        std::map<type_symbol, class_layout> class_layouts;
        std::map<type_symbol, type_placement_info> type_placements;

        RPNX_MEMBER_METADATA(llvm_compilable_unit, target_name, target_code, machine_target, whole_module, whole_module_output_kind, executable_entry_symbol, source_index, inlinable_functions, asm_functions, procedure_linksymbols, antestatal_constants, interface_slots, enum_infos, flagset_infos, class_layouts, type_placements);
    };


}

#endif // QUXLANG_LLVM_BACKEND_TYPES_HPP
