// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/variable_type_spec.hpp>

rpnx::querygraph::coroutine< quxlang::variable_type_spec > quxlang::variable_type_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::query_request< symboid_query >(input);

    if (!typeis< ast2_variable_declaration >(sym))
    {
        throw std::logic_error("Variable not declared.");
    }

    type_symbol var_decl_type = as< ast2_variable_declaration >(sym).type;
    contextual_type_reference ctx_type_ref = {.context = input, .type = var_decl_type};

    auto var_type = co_await rpnx::querygraph::query_request< lookup_query >(ctx_type_ref);

    if (!var_type.has_value())
    {
        throw std::logic_error("Variable type could not be resolved.");
    }

    co_return var_type.value();
}
