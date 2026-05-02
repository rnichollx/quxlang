// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SOURCE_FILE_NAME_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SOURCE_FILE_NAME_SPEC_HEADER_GUARD

#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/source_file_name.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct source_file_name_spec
    {
        using query = source_file_name_query;
        using dependencies = rpnx::typelist< source_file_index_query >;
    };

    rpnx::querygraph::coroutine< source_file_name_spec > source_file_name_impl(std::uint64_t input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SOURCE_FILE_NAME_SPEC_HEADER_GUARD
