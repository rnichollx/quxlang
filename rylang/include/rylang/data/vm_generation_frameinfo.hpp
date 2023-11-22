//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_GENERATION_FRAMEINFO_HEADER
#define RPNX_RYANSCRIPT1031_VM_GENERATION_FRAMEINFO_HEADER

#include "rylang/data/vm_frame_variable.hpp"

namespace rylang
{
    struct vm_generation_block
    {
        std::map< std::string, std::size_t > variable_lookup_index;
        std::map< std::size_t, vm_frame_variable_state > value_states;
        std::optional<qualified_symbol_reference> context_overload;
    };

    struct vm_generation_frame_info
    {
        std::vector< vm_frame_variable > variables;
        std::vector< vm_generation_block > blocks;
        qualified_symbol_reference context;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_GENERATION_FRAMEINFO_HEADER
