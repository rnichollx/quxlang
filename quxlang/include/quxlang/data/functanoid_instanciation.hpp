// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTANOID_INSTANCIATION_HEADER_GUARD
#define QUXLANG_DATA_FUNCTANOID_INSTANCIATION_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    struct functanoid_instanciation
    {
        // The parameter and argument types might be different if the function is a template
        std::vector< type_symbol > parameters;
        std::vector< type_symbol > arguments;
    };
} // namespace quxlang

#endif // FUNCTANOID_INSTANCIATION_HEADER_GUARD
