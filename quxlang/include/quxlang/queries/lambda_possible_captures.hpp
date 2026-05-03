// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LAMBDA_POSSIBLE_CAPTURES_HEADER_GUARD
#define QUXLANG_QUERIES_LAMBDA_POSSIBLE_CAPTURES_HEADER_GUARD

#include <quxlang/data/lambda_types.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <cstddef>
#include <map>
#include <string>

namespace quxlang
{
    struct lambda_possible_captures_subquery
    {
        static constexpr auto subquery_id = "lambda_possible_captures";
        using parent_query = user_vm_procedure3_query;
        using input_type = std::size_t;
        using output_type = std::map< std::string, lambda_possible_capture >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LAMBDA_POSSIBLE_CAPTURES_HEADER_GUARD
