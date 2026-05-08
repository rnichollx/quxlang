// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/module_sources_spec.hpp>

#include <quxlang/data/compilation_result.hpp>

#include <map>

rpnx::querygraph::coroutine< quxlang::module_sources_spec > quxlang::module_sources_impl(std::string input)
{
    source_bundle const bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    std::string const source_name = co_await rpnx::querygraph::request< module_source_name_query >(input);
    std::map< std::string, module_source >::const_iterator const source_iter = bundle.module_sources.find(source_name);
    if (source_iter == bundle.module_sources.end())
    {
        throw quxlang::semantic_compilation_error("Logical module '" + input + "' references missing source module '" + source_name + "'");
    }

    co_return source_iter->second;
}
