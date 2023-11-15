//
// Created by Ryan Nicholl on 11/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_FRAME_INFORMATION_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_FRAME_INFORMATION_HEADER

#include "qualified_symbol_reference.hpp"
#include <string>

namespace rylang
{
    struct function_variable_information
    {
        // TODO: Multiple identifiers?
        std::string identifier;
        qualified_symbol_reference type;
        std::size_t allocation = -1;
        std::size_t allocation_offset = 0;
    };

    struct function_allocation_information
    {
        std::optional<qualified_symbol_reference> type;
        std::optional<std::size_t> size;
        std::optional<std::size_t> alignment;
    };

    struct function_frame_information
    {
        std::vector<function_variable_information> variables;
        std::vector<function_allocation_information> allocations;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_FRAME_INFORMATION_HEADER
