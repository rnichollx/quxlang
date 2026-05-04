// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_IMPLEMENTATION_FUNCTION_MAP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_IMPLEMENTATION_FUNCTION_MAP_SPEC_HEADER_GUARD

#include <quxlang/queries/implementation_function_map.hpp>
#include <quxlang/queries/functanoid_return_type.hpp>
#include <quxlang/queries/implementation_interface_type.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct implementation_function_map_spec
    {
        using query = implementation_function_map_query;
        using dependencies = rpnx::typelist< functanoid_return_type_query, implementation_interface_type_query, instanciation_query, interface_slot_list_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< implementation_function_map_spec > implementation_function_map_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_IMPLEMENTATION_FUNCTION_MAP_SPEC_HEADER_GUARD
