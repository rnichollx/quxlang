// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

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