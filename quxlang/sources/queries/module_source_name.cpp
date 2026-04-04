// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/module_source_name_spec.hpp>

rpnx::querygraph::coroutine< quxlang::module_source_name_spec > quxlang::module_source_name_impl(std::string input)
{
    auto const source_name_map = co_await rpnx::querygraph::query_request< module_source_name_map_query >(std::monostate{});
    co_return source_name_map.at(input);
}
