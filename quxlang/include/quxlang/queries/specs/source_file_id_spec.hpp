// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SOURCE_FILE_ID_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SOURCE_FILE_ID_SPEC_HEADER_GUARD

#include <quxlang/queries/source_file_id.hpp>
#include <quxlang/queries/source_file_index.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using source_file_id_spec = rpnx::querygraph::query_handler_spec< source_file_id_query, rpnx::typelist< source_file_index_query > >;

    rpnx::querygraph::coroutine< source_file_id_spec > source_file_id_impl(source_file_name input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SOURCE_FILE_ID_SPEC_HEADER_GUARD
