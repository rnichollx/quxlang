// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_GLOBAL_IS_STRING_STATIC_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_GLOBAL_IS_STRING_STATIC_SPEC_HEADER_GUARD

#include <quxlang/queries/global_is_string_static.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/variable_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using global_is_string_static_spec = rpnx::querygraph::query_handler_spec<
        global_is_string_static_query,
        rpnx::typelist< symboid_query, symbol_type_query, variable_type_query > >;

    /// Returns true when the symbol is a global STATIC variable whose declared type is STRING_CONSTANT.
    rpnx::querygraph::coroutine< global_is_string_static_spec > global_is_string_static_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_GLOBAL_IS_STRING_STATIC_SPEC_HEADER_GUARD
