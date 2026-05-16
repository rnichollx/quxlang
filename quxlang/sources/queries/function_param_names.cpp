// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_param_names_spec.hpp>
#include "quxlang/exception.hpp"


rpnx::querygraph::coroutine< quxlang::function_param_names_spec > quxlang::function_param_names_impl(temploid_reference input)
{
    QUXLANG_DEBUG_VALUE(quxlang::to_string(input));

    param_names result;
    auto formal_ensig = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(input);
    if (!formal_ensig.has_value())
    {
        throw quxlang::compiler_bug("Formal ensig not found for function parameter name lookup");
    }

    result.positional.resize(formal_ensig->interface.positional.size());

    // Builtin functions don't have any AST to work with, so we can't get the names of the parameters.
    // However, we have to return *something* because the code for argument generation is shared
    // between builtin and non-builtin functions.
    auto is_builtin = co_await rpnx::querygraph::request< function_builtin_query >(input);

    if (is_builtin)
    {
        co_return result;
    }

    std::optional< ast2_function_declaration > decl_opt = co_await rpnx::querygraph::request< function_declaration_query >(input);

    QUXLANG_COMPILER_BUG_IF(!decl_opt.has_value(), "Function declaration not found");

    ast2_function_declaration const& decl = decl_opt.value();

    std::size_t positional_index = 0;
    for (auto const& param : decl.header.call_parameters)
    {
        if (!param.name.has_value())
        {
            continue;
        }
        if (param.api_name.has_value())
        {
            result.named[param.api_name.value()] = param.name.value();
        }
        else
        {
            result.positional[positional_index] = param.name.value();
            positional_index++;
        }
    }

    co_return result;
}
