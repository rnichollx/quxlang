// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INDEXED_SOURCE_BUNDLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INDEXED_SOURCE_BUNDLE_SPEC_HEADER_GUARD

#include <quxlang/queries/indexed_source_bundle.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Defines the source queries required to build the indexed source bundle. */
    struct indexed_source_bundle_spec
    {
        using query = indexed_source_bundle_query;
        using dependencies = rpnx::typelist< source_bundle_query, source_file_index_query >;
    };

    /** Builds the shared indexed view of all source files. */
    rpnx::querygraph::coroutine< indexed_source_bundle_spec > indexed_source_bundle_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INDEXED_SOURCE_BUNDLE_SPEC_HEADER_GUARD
