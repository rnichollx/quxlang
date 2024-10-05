//
// Created by Ryan Nicholl on 1/27/24.
//

#ifndef QUXLANG_DATA_OUTPUT_OBJECT_SYMBOL_HEADER_GUARD
#define QUXLANG_DATA_OUTPUT_OBJECT_SYMBOL_HEADER_GUARD
#include "code_relocation.hpp"


namespace quxlang
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