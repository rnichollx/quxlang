// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTANOID_REQUIRED_STRUCT_LAYOUTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTANOID_REQUIRED_STRUCT_LAYOUTS_SPEC_HEADER_GUARD

#include <quxlang/queries/functanoid_required_struct_layouts.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functanoid_required_struct_layouts_spec
    {
        using query = functanoid_required_struct_layouts_query;
        using dependencies = rpnx::typelist< vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< functanoid_required_struct_layouts_spec > functanoid_required_struct_layouts_impl(functanoid_requirement_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTANOID_REQUIRED_STRUCT_LAYOUTS_SPEC_HEADER_GUARD
