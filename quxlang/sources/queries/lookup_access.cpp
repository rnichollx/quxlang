// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/lookup_spec.hpp>

rpnx::querygraph::coroutine< quxlang::lookup_spec > quxlang::lookup_impl(contextual_type_reference input)
{
    std::optional< type_symbol > const canonical = co_await rpnx::querygraph::request< canonical_lookup_query >(input);
    if (!canonical.has_value())
    {
        co_return std::nullopt;
    }

    bool const accessible = co_await rpnx::querygraph::request< declaration_is_accessible_query >(declaration_access_request{
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
