// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_MODULE_SOURCES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_MODULE_SOURCES_SPEC_HEADER_GUARD

#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/module_source_name.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct module_sources_spec
    {
        using query = module_sources_query;
        using dependencies = rpnx::typelist< source_bundle_query, module_source_name_query >;
    };

    rpnx::querygraph::coroutine< module_sources_spec > module_sources_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_MODULE_SOURCES_SPEC_HEADER_GUARD
