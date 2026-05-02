// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_PARSE_FILE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_PARSE_FILE_SPEC_HEADER_GUARD

#include <quxlang/queries/parse_file.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct parse_file_spec
    {
        using query = parse_file_query;
        using dependencies = rpnx::typelist< source_bundle_query, source_file_index_query >;
    };

    rpnx::querygraph::coroutine< parse_file_spec > parse_file_impl(source_file_name input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_PARSE_FILE_SPEC_HEADER_GUARD
