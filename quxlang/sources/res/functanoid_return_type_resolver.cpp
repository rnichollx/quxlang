//
// Created by Ryan Nicholl on 11/23/23.
//
#include "quxlang/res/functanoid_return_type_resolver.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_return_type)
{
    auto selected_function = co_await QUX_CO_DEP(functum_select_function, (input_val));

    if (!selected_function)
    {
        throw std::logic_error("No function selected");
    }
    auto is_builtin = co_await QUX_CO_DEP(function_builtin, (selected_function.value()));

    if (is_builtin)
    {
        co_return is_builtin.value().return_type;
    }


    auto decl = co_await QUX_CO_DEP(function_declaration, (selected_function.value()));

    if (!decl.has_value())
    {
        throw std::logic_error("No function declaration");
    }

    contextual_type_reference decl_ctx = {.context = input_val, .type = decl.value().definition.return_type.value_or(void_type{})};

    auto decl_type = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (decl_ctx));

    co_return decl_type;
}
