// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_BUILTIN_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_BUILTIN_SPEC_HEADER_GUARD

#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/functum_builtin_overloads.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using function_builtin_spec = rpnx::query_handler_spec< function_builtin_query, rpnx::typelist< functum_builtin_overloads_query > >;

    rpnx::querygraph::coroutine< function_builtin_spec > function_builtin_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_BUILTIN_SPEC_HEADER_GUARD
