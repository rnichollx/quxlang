// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_PROCEDURE_LINKSYMBOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_PROCEDURE_LINKSYMBOL_SPEC_HEADER_GUARD

#include <quxlang/queries/procedure_linksymbol.hpp>
#include <quxlang/queries/symboid.hpp>
#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct procedure_linksymbol_spec
    {
        using query = procedure_linksymbol_query;
        using dependencies = rpnx::typelist< symboid_query >;
    };

    rpnx::querygraph::coroutine< procedure_linksymbol_spec > procedure_linksymbol_impl(ast2_procedure_ref input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_PROCEDURE_LINKSYMBOL_SPEC_HEADER_GUARD
