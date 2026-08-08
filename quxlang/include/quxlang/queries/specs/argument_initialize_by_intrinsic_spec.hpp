// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ARGUMENT_INITIALIZE_BY_INTRINSIC_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ARGUMENT_INITIALIZE_BY_INTRINSIC_SPEC_HEADER_GUARD

#include <quxlang/queries/argument_initialize_by_intrinsic.hpp>
#include <quxlang/queries/interface_defaultable.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct argument_initialize_by_intrinsic_spec
    {
        using query = argument_initialize_by_intrinsic_query;
        using dependencies = rpnx::typelist< interface_defaultable_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< argument_initialize_by_intrinsic_spec > argument_initialize_by_intrinsic_impl(argument_init_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ARGUMENT_INITIALIZE_BY_INTRINSIC_SPEC_HEADER_GUARD
