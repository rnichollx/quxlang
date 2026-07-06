// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/variable_type_spec.hpp>

rpnx::querygraph::coroutine< quxlang::variable_type_spec > quxlang::variable_type_impl(type_symbol input)
{
    if (typeis< subtag_type >(input))
    {
        auto binding = co_await rpnx::querygraph::request< subtag_binding_query >(as< subtag_type >(input));
        if (binding.has_value() && binding->template type_is< parameter_value_instantiation >())
        {
            co_return binding->template get_as< parameter_value_instantiation >().type;
        }
        throw quxlang::compiler_bug("Subtag is not a variable.");
    }

    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);

    if (!typeis< ast2_variable_declaration >(sym))
    {
        throw quxlang::compiler_bug("Variable not declared.");
    }

    type_symbol var_decl_type = as< ast2_variable_declaration >(sym).type;
    contextual_type_reference ctx_type_ref = {.context = input, .type = var_decl_type};

    auto var_type = co_await rpnx::querygraph::request< lookup_query >(ctx_type_ref);

    if (!var_type.has_value())
    {
        throw quxlang::semantic_compilation_error("Variable type could not be resolved.");
    }

    co_return var_type.value();
}
