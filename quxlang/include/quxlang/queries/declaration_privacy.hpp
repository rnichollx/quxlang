// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_DECLARATION_PRIVACY_HEADER_GUARD
#define QUXLANG_QUERIES_DECLARATION_PRIVACY_HEADER_GUARD

#include <quxlang/data/privacy.hpp>

#include <optional>

namespace quxlang
{
    /** Resolves the privacy scope attached to a declaration or selected overload. */
    struct declaration_privacy_query
    {
        static constexpr auto query_id = "declaration_privacy";
        using input_type = type_symbol;
        using output_type = std::optional< resolved_privacy_scope >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_DECLARATION_PRIVACY_HEADER_GUARD
