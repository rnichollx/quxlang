// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_DECLARATION_IS_ACCESSIBLE_HEADER_GUARD
#define QUXLANG_QUERIES_DECLARATION_IS_ACCESSIBLE_HEADER_GUARD

#include <quxlang/data/privacy.hpp>

namespace quxlang
{
    /** Reports whether a selected declaration is accessible from a context. */
    struct declaration_is_accessible_query
    {
        static constexpr auto query_id = "declaration_is_accessible";
        using input_type = declaration_access_request;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_DECLARATION_IS_ACCESSIBLE_HEADER_GUARD
