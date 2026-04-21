// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/templex_initialize_spec.hpp>

rpnx::querygraph::coroutine< quxlang::templex_initialize_spec > quxlang::templex_initialize_impl(initialization_reference input)
{
    auto selection = co_await rpnx::querygraph::request< templex_select_template_query >(input);
    if (!selection)
    {
        QUX_WHY("No matching template found");
        co_return std::nullopt;
    }

    co_return co_await rpnx::querygraph::request< template_instanciation_query >(initialization_reference{
        .initializee = *selection,
        .context = input.context,
        .arguments = input.arguments,
        .parameters = input.parameters,
        .adaptations = input.adaptations,
    });
}
