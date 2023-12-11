//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef FUNCTANOID_INSTANCIATION_HPP
#define FUNCTANOID_INSTANCIATION_HPP

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    struct functanoid_instanciation
    {
        // The parameter and argument types might be different if the function is a template
        std::vector< qualified_symbol_reference > parameters;
        std::vector< qualified_symbol_reference > arguments;
    };
} // namespace rylang

#endif // FUNCTANOID_INSTANCIATION_HPP
