// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/variable.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(variable_type)
{
    auto sym = co_await QUX_CO_DEP(symboid, (input));

    if (!typeis< ast2_variable_declaration >(sym))
    {
        throw std::logic_error("Variable not declared.");
    }

    type_symbol var_decl_type = as< ast2_variable_declaration >(sym).type;
    contextual_type_reference ctx_type_ref = {.context = input, .type = var_decl_type};

    auto var_type = co_await QUX_CO_DEP(lookup, (ctx_type_ref));

    if (!var_type.has_value())
    {
        throw std::logic_error("Variable type could not be resolved.");
    }

    co_return var_type.value();
}
