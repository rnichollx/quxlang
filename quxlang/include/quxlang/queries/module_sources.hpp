// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_MODULE_SOURCES_HEADER_GUARD
#define QUXLANG_QUERIES_MODULE_SOURCES_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <string>

namespace quxlang
{
    struct module_sources_query
    {
        static constexpr auto query_id = "module_sources";
        using input_type = std::string;
        using output_type = module_source;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_MODULE_SOURCES_HEADER_GUARD
