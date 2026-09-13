// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_MANIPULATORS_BODY_CONTEXT_HEADER_GUARD
#define QUXLANG_MANIPULATORS_BODY_CONTEXT_HEADER_GUARD
#include <quxlang/queries/body_parent.hpp>
#include <quxlang/queries/published_name_info.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <rpnx/querygraph/querygraph.hpp>
#include <quxlang/manipulators/body_symbols.hpp>

namespace quxlang
{
    /// Retrieves the explicit lexical parent, or the ordinary parent of a non-body symbol.
    template < typename Coroutine >
    auto co_body_parent(type_symbol context) -> typename Coroutine::template cosubroutine< std::optional< type_symbol > >
    {
        std::optional< std::uint64_t > number = body_number(context);
        if (!number.has_value()) co_return type_parent(context);
        type_symbol owner = as< submember >(context).of;
        std::optional< std::uint64_t > parent = co_await rpnx::querygraph::subquery_request< body_parent >(as< instanciation_reference >(owner), *number);
        co_return parent.has_value() ? body_symbol(owner, *parent) : owner;
    }

    /// Finds the nearest published name without waiting for its procedure to finish.
    template < typename Coroutine >
    auto co_find_body_name(type_symbol context, std::string const& name) -> typename Coroutine::template cosubroutine< std::optional< submember > >
    {
        std::optional< type_symbol > current = std::move(context);
        while (current.has_value())
        {
            if (std::optional< std::uint64_t > number = body_number(*current))
            {
                type_symbol owner = as< submember >(*current).of;
                const auto& names = co_await rpnx::querygraph::subquery_request< published_name_info >(as< instanciation_reference >(owner), *number);
                if (names.contains(name)) co_return submember{.of = *current, .name = name};
            }
            current = co_await co_body_parent< Coroutine >(*current);
        }
        co_return std::nullopt;
    }

    /// Reads one already-resolved declaration from its publication.
    template < typename Coroutine >
    auto co_read_body_name(submember const& symbol) -> typename Coroutine::template cosubroutine< published_name >
    {
        const auto& names = co_await rpnx::querygraph::subquery_request< published_name_info >(
            as< instanciation_reference >(as< submember >(symbol.of).of), body_number(symbol.of).value());
        co_return names.at(symbol.name);
    }

    /// Extracts an object's publication while preserving type-only bindings.
    inline auto published_static_object(published_name const& name) -> std::optional< publish_static_var >
    {
        if (typeis< publish_static_var >(name)) return as< publish_static_var >(name);
        if (typeis< publish_static >(name))
        {
            const auto& binding = as< publish_static >(name).binding;
            if (typeis< publish_static_var >(binding)) return as< publish_static_var >(binding);
        }
        return std::nullopt;
    }

    /// Resolves an object's latest value by storage identity, including shadowed pointer targets.
    template < typename Coroutine >
    auto co_find_body_static(type_symbol context, static_local_ref const& symbol) -> typename Coroutine::template cosubroutine< constexpr_static >
    {
        std::optional< type_symbol > current = std::move(context);
        while (current.has_value())
        {
            if (std::optional< std::uint64_t > number = body_number(*current))
            {
                type_symbol owner = as< submember >(*current).of;
                const auto& names = co_await rpnx::querygraph::subquery_request< published_name_info >(as< instanciation_reference >(owner), *number);
                for (const auto& [name, declaration] : names)
                {
                    std::optional< publish_static_var > object = published_static_object(declaration);
                    if (object.has_value() && object->symbol == symbol) co_return object->object;
                }
            }
            current = co_await co_body_parent< Coroutine >(*current);
        }
        throw semantic_compilation_error("Static object is unavailable: " + symbol.name);
    }
}
#endif
