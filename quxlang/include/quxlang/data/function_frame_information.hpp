// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_FRAME_INFORMATION_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_FRAME_INFORMATION_HEADER_GUARD

#include "type_symbol.hpp"
#include <string>

namespace quxlang
{
    struct function_variable_information
    {
        // TODO: Multiple identifiers?
        std::string identifier;
        type_symbol type;
        std::size_t allocation = -1;
        std::size_t allocation_offset = 0;
    };

    struct function_allocation_information
    {
        std::optional<type_symbol> type;
        std::optional<std::size_t> size;
        std::optional<std::size_t> alignment;
    };

    struct function_frame_information
    {
        std::vector<function_variable_information> variables;
        std::vector<function_allocation_information> allocations;
    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_FRAME_INFORMATION_HEADER_GUARD
