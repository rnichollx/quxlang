// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_TYPE_OPERATION_IS_NOEXCEPT_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_OPERATION_IS_NOEXCEPT_HEADER_GUARD
#include <quxlang/queries/type_operation_is_noexcept.hpp>
#include <quxlang/queries/functum_initialize.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/functanoid_is_noexcept.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Declares dependencies for the exception-contract query. */
    struct type_operation_is_noexcept_spec
    {
        using query = type_operation_is_noexcept_query;
        using dependencies = rpnx::typelist< functum_initialize_query, symbol_type_query, struct_tags_query, functanoid_is_noexcept_query >;
    };
    /** Reports availability and exception behavior of a selected lifecycle operation. */
    rpnx::querygraph::coroutine< type_operation_is_noexcept_spec > type_operation_is_noexcept_impl(expression_is_noexcept_operation input);
}
#endif
