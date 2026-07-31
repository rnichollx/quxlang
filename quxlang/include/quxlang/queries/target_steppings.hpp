// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TARGET_STEPPINGS_HEADER_GUARD
#define QUXLANG_QUERIES_TARGET_STEPPINGS_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <variant>
#include <vector>

namespace quxlang
{
    /** Returns the ordered CPU stepping configurations used by the active target. */
    struct target_steppings_query
    {
        static constexpr auto query_id = "target_steppings";
        using input_type = std::monostate;
        using output_type = std::vector< cpu_stepping_configuration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TARGET_STEPPINGS_HEADER_GUARD
