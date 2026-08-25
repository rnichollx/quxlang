// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLATE_PARAMETER_BINDING_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLATE_PARAMETER_BINDING_SPEC_HEADER_GUARD

#include <quxlang/queries/exists.hpp>
#include <quxlang/queries/subtag_binding.hpp>
#include <quxlang/queries/template_parameter_binding.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Supplies the dependencies for contextual template-parameter binding lookup. */
    struct template_parameter_binding_spec
    {
        using query = template_parameter_binding_query;
        using dependencies = rpnx::typelist< exists_query, subtag_binding_query >;
    };

    /** Finds the nearest unshadowed template parameter binding. */
    rpnx::querygraph::coroutine< template_parameter_binding_spec > template_parameter_binding_impl(template_parameter_binding_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLATE_PARAMETER_BINDING_SPEC_HEADER_GUARD
