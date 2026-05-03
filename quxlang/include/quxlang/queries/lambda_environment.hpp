// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LAMBDA_ENVIRONMENT_HEADER_GUARD
#define QUXLANG_QUERIES_LAMBDA_ENVIRONMENT_HEADER_GUARD

#include <quxlang/data/lambda_types.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <cstddef>

namespace quxlang
{
    struct lambda_environment_subquery
    {
        static constexpr auto subquery_id = "lambda_environment";
        using parent_query = user_vm_procedure3_query;
        using input_type = std::size_t;
        using output_type = lambda_environment;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LAMBDA_ENVIRONMENT_HEADER_GUARD
