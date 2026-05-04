// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INTERFACE_SLOT_LIST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INTERFACE_SLOT_LIST_SPEC_HEADER_GUARD

#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/queries/functanoid_return_type.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct interface_slot_list_spec
    {
        using query = interface_slot_list_query;
        using dependencies = rpnx::typelist< functanoid_return_type_query, instanciation_query, lookup_query, symboid_query >;
    };

    rpnx::querygraph::coroutine< interface_slot_list_spec > interface_slot_list_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INTERFACE_SLOT_LIST_SPEC_HEADER_GUARD
