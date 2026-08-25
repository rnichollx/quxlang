// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/template_parameter_binding_spec.hpp>

rpnx::querygraph::coroutine< quxlang::template_parameter_binding_spec > quxlang::template_parameter_binding_impl(template_parameter_binding_input input)
{
    std::optional< type_symbol > current_context = std::move(input.context);
    while (current_context.has_value())
    {
        if (co_await rpnx::querygraph::request< exists_query >(subsymbol{
                .of = *current_context,
                .name = input.name,
            }))
        {
            co_return std::nullopt;
        }
        if (current_context->template type_is< instanciation_reference >())
        {
            std::optional< parameter_instantiation > binding = co_await rpnx::querygraph::request< subtag_binding_query >(subtag_type{
                .of = *current_context,
                .name = input.name,
            });
            if (binding.has_value())
            {
                co_return binding;
            }
        }
        current_context = type_parent(std::move(*current_context));
    }

    co_return std::nullopt;
}
