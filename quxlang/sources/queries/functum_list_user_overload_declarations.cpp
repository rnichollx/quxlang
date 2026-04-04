// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_list_user_overload_declarations_spec.hpp>

#include "quxlang/operators.hpp"


rpnx::querygraph::coroutine< quxlang::functum_list_user_overload_declarations_spec > quxlang::functum_list_user_overload_declarations_impl(type_symbol input)
{
    std::string name = to_string(input);

    auto const& func_addr = input;

    std::vector< ast2_function_declaration > result;

    auto maybe_functum_ast = co_await rpnx::querygraph::query_request< symboid_query >(input);

    if (!typeis< functum >(maybe_functum_ast))
    {
        // TODO: Redirect to constructor?
        co_return {};
    }

    auto const& functum_v = as< functum >(maybe_functum_ast);

    for (auto const& func : functum_v.functions)
    {
        result.push_back(func);
    }

    co_return result;
}
