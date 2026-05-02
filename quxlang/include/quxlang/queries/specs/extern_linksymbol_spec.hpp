// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_EXTERN_LINKSYMBOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_EXTERN_LINKSYMBOL_SPEC_HEADER_GUARD

#include <quxlang/queries/extern_linksymbol.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct extern_linksymbol_spec
    {
        using query = extern_linksymbol_query;
        using dependencies = rpnx::typelist<  >;
    };

    rpnx::querygraph::coroutine< extern_linksymbol_spec > extern_linksymbol_impl(ast2_extern input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_EXTERN_LINKSYMBOL_SPEC_HEADER_GUARD
