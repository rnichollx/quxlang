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

    if (typeis< submember >(input))
    {
        submember const& member = as< submember >(input);
        auto parent_sym = co_await rpnx::querygraph::request< symboid_query >(member.of);
        if (typeis< ast2_interface_declaration >(parent_sym))
        {
            ast2_interface_declaration const& interface_decl = as< ast2_interface_declaration >(parent_sym);
            for (ast2_interface_function_declaration const& interface_function : interface_decl.functions)
            {
                if (interface_function.name != member.name)
                {
                    continue;
                }
                ast2_function_declaration declaration;
                declaration.header = interface_function.header;
                declaration.definition = interface_function.definition;
                declaration.location = interface_function.location;
                result.push_back(std::move(declaration));
            }
            co_return result;
        }
    }

    auto maybe_functum_ast = co_await rpnx::querygraph::request< symboid_query >(input);

    if (maybe_functum_ast.type_is< ast2_asm_procedure_declaration >())
    {
        ast2_asm_procedure_declaration const& asm_decl = maybe_functum_ast.get_as< ast2_asm_procedure_declaration >();
        if (asm_decl.kind == ast2_asm_declaration_kind::inline_function)
        {
            co_return result;
        }

        for (ast2_asm_callable const& callable : asm_decl.callable_interfaces)
        {
            ast2_function_declaration declaration;
            for (ast2_argument_interface const& argument : callable.args)
            {
                declaration.header.call_parameters.push_back(ast2_function_parameter{
                    .name = std::nullopt,
                    .api_name = argument.api_name,
                    .type = argument.type,
                });
            }
            declaration.definition.return_type = callable.return_type.value_or(type_symbol(void_type{}));
            declaration.location = asm_decl.location;
            result.push_back(std::move(declaration));
        }
        co_return result;
    }

    if (maybe_functum_ast.type_is< ast2_extern_procedure >())
    {
        ast2_extern_procedure const& ext_decl = maybe_functum_ast.get_as< ast2_extern_procedure >();
        if (ext_decl.callable.has_value())
        {
            ast2_asm_callable const& callable = *ext_decl.callable;
            ast2_function_declaration declaration;
            for (ast2_argument_interface const& argument : callable.args)
            {
                declaration.header.call_parameters.push_back(ast2_function_parameter{
                    .name = std::nullopt,
                    .api_name = argument.api_name,
                    .type = argument.type,
                });
            }
            declaration.definition.return_type = callable.return_type.value_or(type_symbol(void_type{}));
            declaration.location = ext_decl.location;
            result.push_back(std::move(declaration));
        }
        co_return result;
    }

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
