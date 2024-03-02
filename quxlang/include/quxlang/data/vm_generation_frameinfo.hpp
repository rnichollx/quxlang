//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef QUXLANG_VM_GENERATION_FRAMEINFO_HEADER_GUARD
#define QUXLANG_VM_GENERATION_FRAMEINFO_HEADER_GUARD

#include "quxlang/data/vm_frame_variable.hpp"

namespace quxlang
{
    struct vm_generation_block
    {
        std::map< std::string, std::size_t > variable_lookup_index;
        std::map< std::size_t, vm_frame_variable_state > value_states;
        std::optional<type_symbol> context_overload;
    };

    struct vm_generation_frame_info
    {
        std::vector< vm_frame_variable > variables;
        std::vector< vm_generation_block > blocks;
        type_symbol context;
    };
} // namespace quxlang

#endif // QUXLANG_VM_GENERATION_FRAMEINFO_HEADER_GUARD
