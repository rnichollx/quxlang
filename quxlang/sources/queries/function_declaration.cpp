// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/function_declaration_spec.hpp>

#include "quxlang/operators.hpp"


rpnx::querygraph::coroutine< quxlang::function_declaration_spec > quxlang::function_declaration_impl(temploid_reference input)
{
    std::optional< builtin_function_info > const& generated = co_await rpnx::querygraph::request< function_primitive_query >(input);
    if (generated.has_value() && generated->execution_kind == function_execution_kind::coroutine)
    {
        ast2_function_declaration declaration;
        declaration.header.execution_kind = function_execution_kind::coroutine;
        declaration.header.controller_parameter = ast2_function_parameter{.api_name = "CONTROLLER", .type = auto_temploidic{}};
        declaration.header.enable_if = generated->overload.enable_if;
        declaration.header.priority = generated->overload.priority;
        declaration.definition.return_type = generated->return_type;
        for (argif const& parameter : generated->overload.interface.positional)
        {
            declaration.header.call_parameters.push_back(ast2_function_parameter{.type = parameter.type, .is_pack = parameter.is_pack});
        }
        for (std::pair< std::string const, argif > const& parameter : generated->overload.interface.named)
        {
            if (parameter.first == "CONTROLLER") continue;
            declaration.header.call_parameters.push_back(ast2_function_parameter{
                .name = parameter.first, .api_name = parameter.first, .type = parameter.second.type, .is_pack = parameter.second.is_pack});
        }
        co_return declaration;
    }
    type_symbol functum = input.templexoid;
    std::string name = to_string(functum);

    auto functum_kind = co_await rpnx::querygraph::request< symbol_type_query >(functum);
    if (functum_kind == symbol_kind::template_)
    {
        throw quxlang::compiler_bug("function_declaration received a template selection. Templates of functions are not directly callable; set template arguments manually and resolve the selected function first.");
    }

    auto const& decls = co_await rpnx::querygraph::request< functum_list_user_overload_declarations_query >(functum);
    auto builtin_overloads = co_await rpnx::querygraph::request< functum_builtin_overloads_query >(functum);
    auto const total_count = decls.size() + builtin_overloads.size();

    if (!input.overload_id.has_value())
    {
        if (total_count != 1 || decls.size() != 1)
        {
            co_return std::nullopt;
        }
        co_return decls.front();
    }

    if (*input.overload_id >= decls.size())
    {
        co_return std::nullopt;
    }

    co_return decls.at(static_cast< std::vector< ast2_function_declaration >::size_type >(*input.overload_id));
}
