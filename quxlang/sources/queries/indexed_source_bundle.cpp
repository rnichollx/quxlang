// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/indexed_source_bundle_spec.hpp>

rpnx::querygraph::coroutine< quxlang::indexed_source_bundle_spec > quxlang::indexed_source_bundle_impl(std::monostate)
{
    source_file_index const& file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    source_bundle const& bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    co_return vmir2::source_index(file_index, bundle);
}
