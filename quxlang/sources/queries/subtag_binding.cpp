// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/subtag_binding_spec.hpp>

rpnx::querygraph::coroutine< quxlang::subtag_binding_spec > quxlang::subtag_binding_impl(subtag_type input)
{
    if (!typeis< instanciation_reference >(input.of))
    {
        co_return std::nullopt;
    }

    std::map< std::string, parameter_instantiation > const bindings = co_await rpnx::querygraph::request< instanciation_subtag_bindings_query >(as< instanciation_reference >(input.of));
    std::map< std::string, parameter_instantiation >::const_iterator const binding = bindings.find(input.name);
    if (binding == bindings.end())
    {
        co_return std::nullopt;
    }

    co_return binding->second;
}
