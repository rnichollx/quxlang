// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLATE_BUILTIN_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLATE_BUILTIN_SPEC_HEADER_GUARD

#include <new>

#include <rpnx/querygraph/querygraph.hpp>

#include <quxlang/queries/template_builtin.hpp>
#include <quxlang/queries/templex_builtins.hpp>

namespace quxlang
{
    struct template_builtin_spec
    {
        using query = template_builtin_query;
        using dependencies = rpnx::typelist< templex_builtins_query >;
    };

    rpnx::querygraph::coroutine< template_builtin_spec > template_builtin_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLATE_BUILTIN_SPEC_HEADER_GUARD
