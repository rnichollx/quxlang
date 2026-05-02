// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SYMBOID_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SYMBOID_SPEC_HEADER_GUARD

#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/declaroids.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/module_ast.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/template_builtin.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct symboid_spec
    {
        using query = symboid_query;
        using dependencies = rpnx::typelist< declaroids_query, instanciation_query, lookup_query, module_ast_query, symbol_type_query, symboid_query, template_builtin_query >;
    };

    rpnx::querygraph::coroutine< symboid_spec > symboid_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SYMBOID_SPEC_HEADER_GUARD
