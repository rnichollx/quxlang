// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/implementation_interface_type_spec.hpp>

#include <quxlang/manipulators/typeutils.hpp>

rpnx::querygraph::coroutine< quxlang::implementation_interface_type_spec > quxlang::implementation_interface_type_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_implementation_declaration >(sym))
    {
        throw std::logic_error("Cannot get interface type of non-implementation: " + quxlang::to_string(input));
    }

    ast2_implementation_declaration const& impl = as< ast2_implementation_declaration >(sym);
    auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
        .context = type_parent(input).value_or(type_symbol(void_type{})),
        .type = impl.interface_type,
    });
    if (!resolved.has_value())
    {
        throw std::logic_error("Implementation interface type could not be resolved");
    }
    if (co_await rpnx::querygraph::request< symbol_type_query >(*resolved) != symbol_kind::interface_)
    {
        throw std::logic_error("IMPLEMENTATION target is not an interface: " + quxlang::to_string(*resolved));
    }
    co_return *resolved;
}
