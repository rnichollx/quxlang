// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTANOID_REQUIRED_TYPE_PLACEMENTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTANOID_REQUIRED_TYPE_PLACEMENTS_SPEC_HEADER_GUARD

#include <quxlang/queries/functanoid_required_type_placements.hpp>
#include <quxlang/queries/vmir_dependencies.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functanoid_required_type_placements_spec
    {
        using query = functanoid_required_type_placements_query;
        using dependencies = rpnx::typelist< vm_procedure3_query, direct_dependencies_query >;
    };

    rpnx::querygraph::coroutine< functanoid_required_type_placements_spec > functanoid_required_type_placements_impl(functanoid_requirement_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTANOID_REQUIRED_TYPE_PLACEMENTS_SPEC_HEADER_GUARD
