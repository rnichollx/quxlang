// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_PARAM_NAMES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_PARAM_NAMES_SPEC_HEADER_GUARD

#include <quxlang/queries/function_param_names.hpp>
#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/function_declaration.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using function_param_names_spec = rpnx::querygraph::query_handler_spec< function_param_names_query, rpnx::typelist< function_builtin_query, function_declaration_query > >;

    rpnx::querygraph::coroutine< function_param_names_spec > function_param_names_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_PARAM_NAMES_SPEC_HEADER_GUARD
