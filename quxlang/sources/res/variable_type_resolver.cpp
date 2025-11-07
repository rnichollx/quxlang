// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/variable.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(variable_type)
{
    auto decls = co_await QUX_CO_DEP(declaroids, (input));

    std::optional<ast2_variable_declaration> var_decl;

    for (declaroid const & x : decls)
    {
        if (!typeis <ast2_variable_declaration>(x))
        {
            throw std::logic_error("Declared as a non-variable.");
        }

        if (var_decl.has_value())
        {
            throw std::logic_error("Variable re-declared in same context.");
        }

        var_decl = as<ast2_variable_declaration>(x);
    }

    if (!var_decl.has_value())
    {
        throw std::logic_error("Variable not declared.");
    }

    type_symbol var_decl_type = var_decl.value().type;
    type_symbol parent_type = type_parent(input).value_or(void_type{});
    contextual_type_reference ctx_type_ref = {.context = parent_type, .type = var_decl_type};

    auto var_type = co_await QUX_CO_DEP(lookup, (ctx_type_ref));

    if (!var_type.has_value())
    {
        throw std::logic_error("Variable type could not be resolved.");
    }

    co_return var_type.value();
}