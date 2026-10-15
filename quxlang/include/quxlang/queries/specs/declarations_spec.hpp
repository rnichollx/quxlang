// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_DECLARATIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_DECLARATIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/declarations.hpp>
#include <quxlang/queries/active_subdeclarations.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct declarations_spec
    {
        using query = declarations_query;
        using dependencies = rpnx::typelist< active_subdeclarations_query >;
    };

    rpnx::querygraph::coroutine< declarations_spec > declarations_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_DECLARATIONS_SPEC_HEADER_GUARD
