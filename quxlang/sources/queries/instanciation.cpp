// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/instanciation_spec.hpp>




rpnx::querygraph::coroutine< quxlang::instanciation_spec > quxlang::instanciation_impl(initialization_reference input)
{
    std::string dbg_input = to_string(input);
    type_symbol templexoid_symbol = input.initializee;

    auto kind = co_await rpnx::querygraph::request< symbol_type_query >(templexoid_symbol);

    std::optional< instanciation_reference > selected;
    if (kind == symbol_kind::functum)
    {
        selected = co_await rpnx::querygraph::request< functum_initialize_query >(input);
    }
    else if (kind == symbol_kind::function)
    {
        selected = co_await rpnx::querygraph::request< function_instanciation_query >(input);
    }
    else if (kind == symbol_kind::template_)
    {
        selected = co_await rpnx::querygraph::request< template_instanciation_query >(input);
    }
    else if (kind == symbol_kind::templex)
    {
        selected = co_await rpnx::querygraph::request< templex_initialize_query >(input);
    }
    else
    {
        co_return std::nullopt;
    }

    if (selected.has_value() && input.context.has_value())
    {
        bool const accessible = co_await rpnx::querygraph::request< declaration_is_accessible_query >(declaration_access_request{
            .accessor_context = *input.context,
            .selected_declaration = *selected,
        });
        if (!accessible)
        {
            throw semantic_compilation_error("Selected overload " + to_string(*selected) + " is private in context " + to_string(*input.context));
        }
    }

    co_return selected;
}
