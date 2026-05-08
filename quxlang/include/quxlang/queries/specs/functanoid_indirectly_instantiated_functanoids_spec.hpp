// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTANOID_INDIRECTLY_INSTANTIATED_FUNCTANOIDS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTANOID_INDIRECTLY_INSTANTIATED_FUNCTANOIDS_SPEC_HEADER_GUARD

#include <quxlang/queries/functanoid_indirectly_instantiated_functanoids.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functanoid_indirectly_instantiated_functanoids_spec
    {
        using query = functanoid_indirectly_instantiated_functanoids_query;
        using dependencies = rpnx::typelist< vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< functanoid_indirectly_instantiated_functanoids_spec > functanoid_indirectly_instantiated_functanoids_impl(functanoid_requirement_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTANOID_INDIRECTLY_INSTANTIATED_FUNCTANOIDS_SPEC_HEADER_GUARD
