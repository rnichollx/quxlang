// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_BODY_SYMBOLS_HEADER_GUARD
#define QUXLANG_MANIPULATORS_BODY_SYMBOLS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <charconv>

namespace quxlang
{
    /// Recognizes a compiler-generated body submember.
    inline auto body_number(type_symbol const& symbol) -> std::optional< std::uint64_t >
    {
        if (!typeis< submember >(symbol)) return std::nullopt;
        std::string const& name = as< submember >(symbol).name;
        std::string_view prefix = "__BODY";
        if (!name.starts_with(prefix)) return std::nullopt;
        std::uint64_t number = 0;
        auto parsed = std::from_chars(name.data() + prefix.size(), name.data() + name.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != name.data() + name.size()) return std::nullopt;
        return number;
    }

    /// Forms a body context owned by an instantiated VM procedure.
    inline auto body_symbol(type_symbol owner, std::uint64_t number) -> type_symbol
    {
        return submember{.of = std::move(owner), .name = "__BODY" + std::to_string(number)};
    }

} // namespace quxlang

#endif // QUXLANG_MANIPULATORS_BODY_SYMBOLS_HEADER_GUARD
