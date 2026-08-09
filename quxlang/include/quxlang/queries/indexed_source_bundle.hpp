// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INDEXED_SOURCE_BUNDLE_HEADER_GUARD
#define QUXLANG_QUERIES_INDEXED_SOURCE_BUNDLE_HEADER_GUARD

#include <quxlang/vmir2/source_index.hpp>

#include <variant>

namespace quxlang
{
    /** Provides the source bundle indexed by deterministic source file IDs. */
    struct indexed_source_bundle_query
    {
        static constexpr auto query_id = "indexed_source_bundle";
        using input_type = std::monostate;
        using output_type = vmir2::source_index;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INDEXED_SOURCE_BUNDLE_HEADER_GUARD
