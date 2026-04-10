// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_MODULE_SOURCE_NAME_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_MODULE_SOURCE_NAME_SPEC_HEADER_GUARD

#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/module_source_name.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using module_source_name_spec = rpnx::querygraph::query_handler_spec< module_source_name_query, rpnx::typelist< module_source_name_map_query > >;

    rpnx::querygraph::coroutine< module_source_name_spec > module_source_name_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_MODULE_SOURCE_NAME_SPEC_HEADER_GUARD
