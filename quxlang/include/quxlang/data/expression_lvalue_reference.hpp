// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_EXPRESSION_LVALUE_REFERENCE_HEADER_GUARD
#define QUXLANG_DATA_EXPRESSION_LVALUE_REFERENCE_HEADER_GUARD

#include "quxlang/data/lookup_chain.hpp"

namespace quxlang
{
    struct expression_lvalue_reference
    {
        lookup_chain chain;
    };
} // namespace quxlang

#endif // QUXLANG_EXPRESSION_LVALUE_REFERENCE_HEADER_GUARD
