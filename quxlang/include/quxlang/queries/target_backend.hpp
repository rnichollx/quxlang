// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TARGET_BACKEND_HEADER_GUARD
#define QUXLANG_QUERIES_TARGET_BACKEND_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <variant>

namespace quxlang
{
    /// target_backend_query returns the configured backend kind for the active target.
    struct target_backend_query
    {
        static constexpr auto query_id = "target_backend";
        using input_type = std::monostate;
        using output_type = backend_kind;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TARGET_BACKEND_HEADER_GUARD
