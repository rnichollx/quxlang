// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LAMBDA_CAPTURE_SET_HEADER_GUARD
#define QUXLANG_QUERIES_LAMBDA_CAPTURE_SET_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <cstddef>
#include <vector>

namespace quxlang
{
    struct lambda_capture_set_subquery
    {
        static constexpr auto subquery_id = "lambda_capture_set";
        using parent_query = user_vm_procedure3_query;
        using input_type = std::size_t;
        using output_type = std::vector< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LAMBDA_CAPTURE_SET_HEADER_GUARD
