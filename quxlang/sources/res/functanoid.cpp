// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/functanoid.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/compiler_binding.hpp"
#include "quxlang/exception.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_return_type)
{
    std::string input_str = quxlang::to_string(input);
    temploid_reference selected_function = input.temploid;

    auto primitive = co_await QUX_CO_DEP(function_primitive, (selected_function));

    if (primitive)
    {
        co_return primitive.value().return_type;
    }

    auto decl = co_await QUX_CO_DEP(function_declaration, (selected_function));

    if (!decl.has_value())
    {
        throw std::logic_error("No function declaration");
    }

    contextual_type_reference decl_ctx = {.context = input_val, .type = decl.value().definition.return_type.value_or(void_type{})};

    auto decl_type = co_await QUX_CO_DEP(lookup, (decl_ctx));

    co_return decl_type.value();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_sigtype)
{
    assert(!type_is_contextual(input));
    sigtype result;

    result.params = input.params;

    result.return_type = co_await QUX_CO_DEP(functanoid_return_type, (input));

    co_return result;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_param_names)
{
    // Transitional: use compiler_binder.
    compiler_binder prv(c);

    QUXLANG_DEBUG_VALUE(quxlang::to_string(input));

    param_names result;

    result.positional.resize(input.which.interface.positional.size());

    // Builtin functions don't have any AST to work with, so we can't get the names of the parameters.
    // However, we have to return *something* because the code for argument generation is shared
    // between builtin and non-builtin functions.
    auto is_builtin = co_await prv.function_builtin(input);

    if (is_builtin)
    {
        co_return result;
    }

    std::optional< ast2_function_declaration > decl_opt = co_await prv.function_declaration(input);

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
