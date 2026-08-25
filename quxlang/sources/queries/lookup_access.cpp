// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/lookup_spec.hpp>

rpnx::querygraph::coroutine< quxlang::lookup_spec > quxlang::lookup_impl(contextual_type_reference input)
{
    std::optional< type_symbol > canonical = co_await rpnx::querygraph::request< canonical_lookup_query >(input);
    if (!canonical.has_value())
    {
        co_return std::nullopt;
    }

    if (input.type.type_is< freebound_identifier >())
    {
        freebound_identifier const& identifier = input.type.get_as< freebound_identifier >();
        std::optional< parameter_instantiation > binding = co_await rpnx::querygraph::request< template_parameter_binding_query >(template_parameter_binding_input{
            .context = input.context,
            .name = identifier.name,
        });
        if (binding.has_value() && binding->type_is< parameter_type_instantiation >())
        {
            type_symbol const& bound_type = binding->get_as< parameter_type_instantiation >().type;
            if (*canonical == bound_type)
            {
                co_return canonical;
            }
        }
    }

    bool accessible = co_await rpnx::querygraph::request< declaration_is_accessible_query >(declaration_access_request{
        .accessor_context = input.context,
        .selected_declaration = *canonical,
        .kind = declaration_access_kind::lookup_path,
    });
    if (!accessible)
    {
        throw semantic_compilation_error("Lookup of " + to_string(*canonical) + " is inaccessible in context " + to_string(input.context));
    }

    co_return canonical;
}
