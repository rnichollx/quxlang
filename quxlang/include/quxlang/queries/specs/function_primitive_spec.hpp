// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_PRIMITIVE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_PRIMITIVE_SPEC_HEADER_GUARD

#include <quxlang/queries/function_primitive.hpp>
#include <quxlang/queries/functum_builtins.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using function_primitive_spec = rpnx::querygraph::query_handler_spec< function_primitive_query, rpnx::typelist< functum_builtins_query > >;

    rpnx::querygraph::coroutine< function_primitive_spec > function_primitive_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_PRIMITIVE_SPEC_HEADER_GUARD
