//
// Created by Ryan Nicholl on 1/27/24.
//

#ifndef OUTPUT_OBJECT_SYMBOL_HPP
#define OUTPUT_OBJECT_SYMBOL_HPP
#include "code_relocation.hpp"


namespace rylang
{

    struct object_symbol
    {
        std::vector< symbol_relocation > relocations;
    };
}

#endif //OUTPUT_OBJECT_SYMBOL_HPP