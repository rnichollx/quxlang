// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INTERFACE_DEFAULTABLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INTERFACE_DEFAULTABLE_SPEC_HEADER_GUARD

#include <quxlang/queries/interface_defaultable.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct interface_defaultable_spec
    {
        using query = interface_defaultable_query;
        using dependencies = rpnx::typelist< symboid_query >;
    };

    rpnx::querygraph::coroutine< interface_defaultable_spec > interface_defaultable_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INTERFACE_DEFAULTABLE_SPEC_HEADER_GUARD
