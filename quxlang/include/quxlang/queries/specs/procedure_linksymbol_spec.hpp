// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_PROCEDURE_LINKSYMBOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_PROCEDURE_LINKSYMBOL_SPEC_HEADER_GUARD

#include <quxlang/queries/procedure_linksymbol.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using procedure_linksymbol_spec = rpnx::querygraph::query_handler_spec< procedure_linksymbol_query, rpnx::typelist<  > >;

    rpnx::querygraph::coroutine< procedure_linksymbol_spec > procedure_linksymbol_impl(ast2_procedure_ref input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_PROCEDURE_LINKSYMBOL_SPEC_HEADER_GUARD
