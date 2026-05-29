// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_LLVM_BACKEND_TYPES_HPP
#define QUXLANG_LLVM_BACKEND_TYPES_HPP

#include "data/machine.hpp"

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/class_layout.hpp>
#include <quxlang/data/enum_flagset_info.hpp>
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

        RPNX_MEMBER_METADATA(llvm_compiled_unit, bitcode, llvm_ir_text, optimized_llvm_ir_text);
    };

    /// llvm_compilation_target represents the compilation information needed to compile something to llvm
    struct llvm_compilation_target
    {
        output_info machine;
        optimization_level optimization = optimization_level::debug;

        RPNX_MEMBER_METADATA(llvm_compilation_target, machine, optimization);
    };

    /// llvm_compilable_unit represents a unit of code for llvm to compile with all required input information
    struct llvm_compilable_unit
    {
        type_symbol target_name;
        vmir2::functanoid_routine3 target_code;
        llvm_compilation_target machine_target;
        std::optional< rpnx::cow< vmir2::source_index > > source_index;
        std::map<type_symbol, vmir2::functanoid_routine3> inlinable_functions;
        std::map<type_symbol, antestatal_value> antestatal_constants;
        std::map<type_symbol, std::vector< interface_slot_key > > interface_slots;
        std::map<type_symbol, enum_info> enum_infos;
        std::map<type_symbol, flagset_info> flagset_infos;
        std::map<type_symbol, class_layout> class_layouts;
        std::map<type_symbol, type_placement_info> type_placements;

        RPNX_MEMBER_METADATA(llvm_compilable_unit, target_name, target_code, machine_target, source_index, inlinable_functions, antestatal_constants, interface_slots, enum_infos, flagset_infos, class_layouts, type_placements);
    };


}

#endif // QUXLANG_LLVM_BACKEND_TYPES_HPP
