// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_COROUTINE_TYPES_HEADER_GUARD
#define QUXLANG_DATA_COROUTINE_TYPES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <optional>
#include <string>

namespace quxlang
{
    /** Returns the nominal frame identity of a concrete coroutine specialization. */
    inline auto coroutine_frame_symbol(instanciation_reference const& coroutine) -> type_symbol
    {
        return subsymbol{.of = coroutine, .name = "FRAME_TYPE"};
    }

    /** Recognizes a nominal frame without requesting its physical layout. */
    inline auto coroutine_frame_specialization(type_symbol const& symbol) -> std::optional< instanciation_reference >
    {
        if (!symbol.type_is< subsymbol >()) return std::nullopt;
        subsymbol const& member = symbol.get_as< subsymbol >();
        if (member.name != "FRAME_TYPE" || !member.of.type_is< instanciation_reference >()) return std::nullopt;
        return member.of.get_as< instanciation_reference >();
    }

    /** Identifies a compiler-generated operation on a coroutine frame. */
    struct coroutine_frame_operation
    {
        instanciation_reference coroutine;
        std::string name;
    };

    /** Recognizes a frame operation before resolving its generated declaration. */
    inline auto coroutine_frame_operation_symbol(type_symbol const& symbol) -> std::optional< coroutine_frame_operation >
    {
        if (!symbol.type_is< submember >()) return std::nullopt;
        submember const& member = symbol.get_as< submember >();
        std::optional< instanciation_reference > coroutine = coroutine_frame_specialization(member.of);
        if (!coroutine.has_value()) return std::nullopt;
        return coroutine_frame_operation{.coroutine = std::move(*coroutine), .name = member.name};
    }

    /** Returns the private typed-storage field assigned to a logical local. */
    inline auto coroutine_storage_field_name(std::uint64_t local) -> std::string
    {
        return "__LOCAL" + std::to_string(local);
    }
}

#endif // QUXLANG_DATA_COROUTINE_TYPES_HEADER_GUARD
