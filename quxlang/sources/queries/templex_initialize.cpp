// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/templex_initialize_spec.hpp>

rpnx::querygraph::coroutine< quxlang::templex_initialize_spec > quxlang::templex_initialize_impl(initialization_reference input)
{
    auto initializee_kind = co_await rpnx::querygraph::request< symbol_type_query >(input.initializee);

    if (initializee_kind != symbol_kind::templex && initializee_kind != symbol_kind::template_)
    {
        throw std::logic_error("templex_initialize called on a non-templexoid template input");
    }

    co_return co_await rpnx::querygraph::request< template_instanciation_query >(input);
}
