// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONFIGURED_TARGET_HEADER_GUARD
#define QUXLANG_QUERIES_CONFIGURED_TARGET_HEADER_GUARD

#include <string>
#include <variant>

namespace quxlang
{
    /** Identifies the active target by its source-bundle configuration key. */
    struct configured_target_query
    {
        static constexpr auto query_id = "configured_target";
        using input_type = std::monostate;
        using output_type = std::string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONFIGURED_TARGET_HEADER_GUARD
