// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_MODULE_OPTIONS_MAP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_MODULE_OPTIONS_MAP_SPEC_HEADER_GUARD

#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/module_option_strings_map.hpp>
#include <quxlang/queries/module_options_map.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <rpnx/querygraph/querygraph.hpp>

#include <variant>

namespace quxlang
{
    using module_options_map_spec = rpnx::querygraph::query_handler_spec< module_options_map_query, rpnx::typelist< module_option_strings_map_query, lookup_query, symbol_type_query > >;

    rpnx::querygraph::coroutine< module_options_map_spec > module_options_map_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_MODULE_OPTIONS_MAP_SPEC_HEADER_GUARD
