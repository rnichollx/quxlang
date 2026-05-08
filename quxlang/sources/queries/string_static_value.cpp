// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/string_static_value_spec.hpp>

#include <stdexcept>

/// Resolves a STATIC STRING_CONSTANT initializer, including references to other string statics, into concrete bytes.
rpnx::querygraph::coroutine< quxlang::string_static_value_spec > quxlang::string_static_value_impl(type_symbol input)
{
    if (!(co_await rpnx::querygraph::request< global_is_string_static_query >(input)))
    {
        throw quxlang::semantic_compilation_error("requested string value for a non-string static: " + quxlang::to_string(input));
    }

    auto symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        throw quxlang::semantic_compilation_error("string static symbol is not a variable: " + quxlang::to_string(input));
    }

    auto const& decl = as< ast2_variable_declaration >(symboid);
    if (!decl.init_expr.has_value())
    {
        throw quxlang::semantic_compilation_error("STATIC STRING_CONSTANT requires a := initializer: " + quxlang::to_string(input));
    }
    if (!decl.init_args.empty())
    {
        throw quxlang::semantic_compilation_error("STATIC STRING_CONSTANT initializer argument lists are not implemented: " + quxlang::to_string(input));
    }

    if (typeis< expression_symbol_reference >(*decl.init_expr))
    {
        auto const& symbol_reference = as< expression_symbol_reference >(*decl.init_expr);
        auto referenced_symbol = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input, .type = symbol_reference.symbol});
        if (referenced_symbol.has_value() && *referenced_symbol != input && (co_await rpnx::querygraph::request< global_is_string_static_query >(*referenced_symbol)))
        {
            auto referenced_symboid = co_await rpnx::querygraph::request< symboid_query >(*referenced_symbol);
            if (!typeis< ast2_variable_declaration >(referenced_symboid))
            {
                throw quxlang::semantic_compilation_error("referenced string static symbol is not a variable: " + quxlang::to_string(*referenced_symbol));
            }

            auto const& referenced_decl = as< ast2_variable_declaration >(referenced_symboid);
            if (!referenced_decl.init_expr.has_value())
            {
                throw quxlang::semantic_compilation_error("referenced STATIC STRING_CONSTANT requires a := initializer: " + quxlang::to_string(*referenced_symbol));
            }
            if (!referenced_decl.init_args.empty())
            {
                throw quxlang::semantic_compilation_error("referenced STATIC STRING_CONSTANT initializer argument lists are not implemented: " + quxlang::to_string(*referenced_symbol));
            }

            constexpr_input_v3 referenced_constexpr_input;
            referenced_constexpr_input.expr = *referenced_decl.init_expr;
            referenced_constexpr_input.context = *referenced_symbol;
            co_return co_await rpnx::querygraph::request< constexpr_eval_string_query >(std::move(referenced_constexpr_input));
        }
    }

    constexpr_input_v3 constexpr_input;
    constexpr_input.expr = *decl.init_expr;
    constexpr_input.context = input;

    co_return co_await rpnx::querygraph::request< constexpr_eval_string_query >(std::move(constexpr_input));
}
