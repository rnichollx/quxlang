// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/module_source_name_spec.hpp>

#include <quxlang/data/compilation_result.hpp>

#include <map>

rpnx::querygraph::coroutine< quxlang::module_source_name_spec > quxlang::module_source_name_impl(std::string input)
{
    std::map< std::string, std::string > const source_name_map = co_await rpnx::querygraph::request< module_source_name_map_query >(std::monostate{});
    std::map< std::string, std::string >::const_iterator const source_iter = source_name_map.find(input);
    if (source_iter == source_name_map.end())
    {
        throw quxlang::semantic_compilation_error("Unknown logical module '" + input + "'");
    }

    co_return source_iter->second;
}
