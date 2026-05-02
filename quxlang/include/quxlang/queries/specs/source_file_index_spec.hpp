// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SOURCE_FILE_INDEX_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SOURCE_FILE_INDEX_SPEC_HEADER_GUARD

#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct source_file_index_spec
    {
        using query = source_file_index_query;
        using dependencies = rpnx::typelist< source_bundle_query >;
    };

    rpnx::querygraph::coroutine< source_file_index_spec > source_file_index_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SOURCE_FILE_INDEX_SPEC_HEADER_GUARD
