// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/antestatal_static_value_spec.hpp>

#include <stdexcept>


rpnx::querygraph::coroutine< quxlang::antestatal_static_value_spec > quxlang::antestatal_static_value_impl(type_symbol input)
{
    if (!(co_await rpnx::querygraph::request< global_is_antestatal_static_query >(input)))
    {
        throw quxlang::semantic_compilation_error("requested antestatal value for a non-antestatal static: " + quxlang::to_string(input));
    }

    auto symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        throw quxlang::semantic_compilation_error("antestatal static symbol is not a variable: " + quxlang::to_string(input));
    }

    auto const& decl = as< ast2_variable_declaration >(symboid);
    if (!decl.init_expr.has_value())
    {
        throw quxlang::semantic_compilation_error("antestatal STATIC requires a := initializer: " + quxlang::to_string(input));
    }
    if (!decl.init_args.empty())
    {
        throw quxlang::semantic_compilation_error("antestatal STATIC initializer argument lists are not implemented: " + quxlang::to_string(input));
    }

    auto variable_type = co_await rpnx::querygraph::request< variable_type_query >(input);

    constexpr_input2 constexpr_input;
    constexpr_input.expr = *decl.init_expr;
    constexpr_input.context = input;
    constexpr_input.type = variable_type;
    constexpr_input.antestatal_global_symbol = input;

    co_return co_await rpnx::querygraph::request< constexpr_eval_antestatal_query >(constexpr_input);
}
