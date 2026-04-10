// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_EXTERN_LINKSYMBOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_EXTERN_LINKSYMBOL_SPEC_HEADER_GUARD

#include <quxlang/queries/extern_linksymbol.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using extern_linksymbol_spec = rpnx::querygraph::query_handler_spec< extern_linksymbol_query, rpnx::typelist<  > >;

    rpnx::querygraph::coroutine< extern_linksymbol_spec > extern_linksymbol_impl(ast2_extern input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_EXTERN_LINKSYMBOL_SPEC_HEADER_GUARD
