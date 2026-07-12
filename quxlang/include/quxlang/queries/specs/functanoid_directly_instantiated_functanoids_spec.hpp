// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTANOID_DIRECTLY_INSTANTIATED_FUNCTANOIDS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTANOID_DIRECTLY_INSTANTIATED_FUNCTANOIDS_SPEC_HEADER_GUARD

#include <quxlang/queries/functanoid_directly_instantiated_functanoids.hpp>
#include <quxlang/queries/vm_procedure3.hpp>
#include <quxlang/queries/vmir_dependencies.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functanoid_directly_instantiated_functanoids_spec
    {
        using query = functanoid_directly_instantiated_functanoids_query;
        using dependencies = rpnx::typelist< vm_procedure3_query, direct_dependencies_query >;
    };

    rpnx::querygraph::coroutine< functanoid_directly_instantiated_functanoids_spec > functanoid_directly_instantiated_functanoids_impl(functanoid_requirement_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTANOID_DIRECTLY_INSTANTIATED_FUNCTANOIDS_SPEC_HEADER_GUARD
