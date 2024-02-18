//
// Created by Ryan Nicholl on 1/27/24.
//

#ifndef RYLANG_OUTPUT_OBJECT_SYMBOL_HPP
#define RYLANG_OUTPUT_OBJECT_SYMBOL_HPP
#include "code_relocation.hpp"


namespace rylang
{
    struct object_symbol
    {
        std::string name;
        std::string section;
        std::vector<std::byte> data;
        std::vector< symbol_relocation > relocations;
    };
}

#endif //OUTPUT_OBJECT_SYMBOL_HPP