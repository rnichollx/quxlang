//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef RYLANG_FUNCTANOID_INSTANCIATION_HEADER_GUARD
#define RYLANG_FUNCTANOID_INSTANCIATION_HEADER_GUARD

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    struct functanoid_instanciation
    {
        // The parameter and argument types might be different if the function is a template
        std::vector< type_symbol > parameters;
        std::vector< type_symbol > arguments;
    };
} // namespace rylang

#endif // FUNCTANOID_INSTANCIATION_HEADER_GUARD
