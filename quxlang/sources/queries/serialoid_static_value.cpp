// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/serialoid_static_value_spec.hpp>

#include <stdexcept>

rpnx::querygraph::coroutine< quxlang::serialoid_static_value_spec > quxlang::serialoid_static_value_impl(type_symbol input)
{
    if (!(co_await rpnx::querygraph::request< global_is_serialoid_static_query >(input)))
    {
        throw std::logic_error("requested serialoid value for a non-serialoid static: " + quxlang::to_string(input));
    }

    auto symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        throw std::logic_error("serialoid static symbol is not a variable: " + quxlang::to_string(input));
    }

    auto const& decl = as< ast2_variable_declaration >(symboid);
    if (!decl.init_expr.has_value())
    {
        throw std::logic_error("serialoid STATIC requires a := initializer: " + quxlang::to_string(input));
    }
    if (!decl.init_args.empty())
    {
        throw std::logic_error("serialoid STATIC initializer argument lists are not implemented: " + quxlang::to_string(input));
    }

    auto variable_type = co_await rpnx::querygraph::request< variable_type_query >(input);

    constexpr_input_v3 constexpr_input;
    constexpr_input.expr = *decl.init_expr;
    constexpr_input.context = input;
    constexpr_input.expected_result_type = variable_type;

    auto result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(std::move(constexpr_input));
    auto result_it = result.values.find(constexpr_primary_result_id);
    if (result_it == result.values.end() || !typeis< constexpr_serialoid >(result_it->second))
    {
        throw std::logic_error("serialoid STATIC did not produce a serialized value: " + quxlang::to_string(input));
    }

    co_return as< constexpr_serialoid >(std::move(result_it->second));
}
