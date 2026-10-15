// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_TYPE_OPERATION_IS_NOEXCEPT_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_OPERATION_IS_NOEXCEPT_HEADER_GUARD
#include <quxlang/data/basic_types.hpp>
namespace quxlang
{
    /** Reports availability and exception behavior of a selected lifecycle operation. */
    struct type_operation_is_noexcept_query
    {
        static constexpr auto query_id = "type_operation_is_noexcept";
        using input_type = expression_is_noexcept_operation;
        using output_type = bool;
    };
}
#endif
