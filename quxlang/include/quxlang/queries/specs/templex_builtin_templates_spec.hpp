// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLEX_BUILTIN_TEMPLATES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLEX_BUILTIN_TEMPLATES_SPEC_HEADER_GUARD

#include <new>

#include <rpnx/querygraph/querygraph.hpp>

#include <quxlang/queries/templex_builtin_templates.hpp>
#include <quxlang/queries/templex_builtins.hpp>

namespace quxlang
{
    using templex_builtin_templates_spec =
        rpnx::querygraph::query_handler_spec< templex_builtin_templates_query, rpnx::typelist< templex_builtins_query > >;

    rpnx::querygraph::coroutine< templex_builtin_templates_spec > templex_builtin_templates_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLEX_BUILTIN_TEMPLATES_SPEC_HEADER_GUARD
