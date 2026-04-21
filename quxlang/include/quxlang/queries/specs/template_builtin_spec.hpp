// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLATE_BUILTIN_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLATE_BUILTIN_SPEC_HEADER_GUARD

#include <new>

#include <rpnx/querygraph/querygraph.hpp>

#include <quxlang/queries/template_builtin.hpp>
#include <quxlang/queries/templex_builtin_templates.hpp>

namespace quxlang
{
    using template_builtin_spec = rpnx::querygraph::query_handler_spec< template_builtin_query, rpnx::typelist< templex_builtin_templates_query > >;

    rpnx::querygraph::coroutine< template_builtin_spec > template_builtin_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLATE_BUILTIN_SPEC_HEADER_GUARD
