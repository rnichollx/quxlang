// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ANTESTATAL_STATIC_VALUE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ANTESTATAL_STATIC_VALUE_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/constexpr_eval_antestatal.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/variable_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using antestatal_static_value_spec = rpnx::querygraph::query_handler_spec< antestatal_static_value_query, rpnx::typelist< constexpr_eval_antestatal_query, global_is_antestatal_static_query, symboid_query, variable_type_query > >;

    rpnx::querygraph::coroutine< antestatal_static_value_spec > antestatal_static_value_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ANTESTATAL_STATIC_VALUE_SPEC_HEADER_GUARD
