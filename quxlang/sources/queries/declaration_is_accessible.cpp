// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/declaration_is_accessible_spec.hpp>

#include <utility>

namespace quxlang
{
    /** Returns whether a context is equal to or nested within an ancestor context. */
    static auto context_is_within(type_symbol context, type_symbol const& ancestor) -> bool
    {
        std::optional< type_symbol > current = std::move(context);
        while (current.has_value())
        {
            if (*current == ancestor)
            {
                return true;
            }
            current = type_parent(*current);
        }
        return false;
    }

    /** Returns the canonical declaration symbol whose lexical parents form a lookup path. */
    static auto lookup_path_declaration(type_symbol declaration) -> type_symbol
    {
        if (declaration.type_is< initialization_reference >())
        {
            declaration = declaration.get_as< initialization_reference >().initializee;
        }
        if (declaration.type_is< instanciation_reference >())
        {
            declaration = declaration.get_as< instanciation_reference >().temploid;
        }
        if (declaration.type_is< temploid_reference >())
        {
            declaration = declaration.get_as< temploid_reference >().templexoid;
        }
        return declaration;
    }
} // namespace quxlang

rpnx::querygraph::coroutine< quxlang::declaration_is_accessible_spec > quxlang::declaration_is_accessible_impl(declaration_access_request input)
{
    std::optional< type_symbol > current_declaration = input.selected_declaration;
    bool is_selected_declaration = true;
    while (current_declaration.has_value())
    {
        std::optional< resolved_privacy_scope > const privacy = co_await rpnx::querygraph::request< declaration_privacy_query >(*current_declaration);
        if (privacy.has_value())
        {
            bool accessible = context_is_within(input.accessor_context, *current_declaration);
            for (type_symbol const& allowed_context : privacy->contexts)
            {
                accessible = accessible || context_is_within(input.accessor_context, allowed_context);
            }
            if (!accessible)
            {
                co_return false;
            }
        }

        if (input.kind == declaration_access_kind::selected_declaration)
        {
            co_return true;
        }

        type_symbol const lookup_declaration = is_selected_declaration ? lookup_path_declaration(*current_declaration) : *current_declaration;
        current_declaration = type_parent(lookup_declaration);
        is_selected_declaration = false;
    }
    co_return true;
}
