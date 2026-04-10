// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/module_sources_spec.hpp>

rpnx::querygraph::coroutine< quxlang::module_sources_spec > quxlang::module_sources_impl(std::string input)
{
    source_bundle const bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    std::string const source_name = co_await rpnx::querygraph::request< module_source_name_query >(input);
    co_return bundle.module_sources.at(source_name);
}
