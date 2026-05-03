// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LAMBDA_OPERATOR_HEADER_GUARD
#define QUXLANG_QUERIES_LAMBDA_OPERATOR_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <cstddef>

namespace quxlang
{
    struct lambda_operator_subquery
    {
        static constexpr auto subquery_id = "lambda_operator";
        using parent_query = user_vm_procedure3_query;
        using input_type = std::size_t;
        using output_type = ast2_function_declaration;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LAMBDA_OPERATOR_HEADER_GUARD
