// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_list_user_overload_declarations_spec.hpp>

#include <quxlang/data/lambda_types.hpp>
#include "quxlang/operators.hpp"


rpnx::querygraph::coroutine< quxlang::functum_list_user_overload_declarations_spec > quxlang::functum_list_user_overload_declarations_impl(type_symbol input)
{
    std::string name = to_string(input);

    auto const& func_addr = input;

    std::vector< ast2_function_declaration > result;

    if (auto lambda = parse_lambda_operator_symbol(input); lambda.has_value())
    {
        result.push_back(co_await rpnx::querygraph::subquery_request< lambda_operator_subquery >(as< instanciation_reference >(lambda->parent_functanoid), lambda->index));
        co_return result;
    }

    if (auto lambda = parse_lambda_constructor_symbol(input); lambda.has_value())
    {
        auto captures = co_await rpnx::querygraph::subquery_request< lambda_capture_set_subquery >(as< instanciation_reference >(lambda->parent_functanoid), lambda->index);
        ast2_function_declaration declaration;
        for (std::size_t i = 0; i < captures.size(); i++)
        {
            declaration.header.call_parameters.push_back(ast2_function_parameter{
                .name = "__CAPTURE_ARG" + std::to_string(i),
                .api_name = std::nullopt,
                .type = captures.at(i),
            });
        }
        declaration.definition.return_type = void_type{};
        result.push_back(std::move(declaration));
        co_return result;
    }

    auto maybe_functum_ast = co_await rpnx::querygraph::request< symboid_query >(input);

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
