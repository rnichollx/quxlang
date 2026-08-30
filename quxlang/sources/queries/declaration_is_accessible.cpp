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

} // namespace quxlang

rpnx::querygraph::coroutine< quxlang::declaration_is_accessible_spec > quxlang::declaration_is_accessible_impl(declaration_access_request input)
{
    std::optional< resolved_privacy_scope > privacy = co_await rpnx::querygraph::request< declaration_privacy_query >(input.selected_declaration);
    if (!privacy.has_value())
    {
        co_return true;
    }

    bool accessible = context_is_within(input.accessor_context, input.selected_declaration);
    for (type_symbol const& allowed_context : privacy->contexts)
    {
        accessible = accessible || context_is_within(input.accessor_context, allowed_context);
    }
    co_return accessible;
}
