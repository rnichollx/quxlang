// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ENSIG_ARGUMENT_INITIALIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ENSIG_ARGUMENT_INITIALIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/ensig_argument_initialize.hpp>
#include <quxlang/queries/argument_initialize_by_class_conversion.hpp>
#include <quxlang/queries/argument_initialize_by_intrinsic.hpp>
#include <quxlang/queries/argument_initialize_by_template.hpp>
#include <quxlang/queries/bindable.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct ensig_argument_initialize_spec
    {
        using query = ensig_argument_initialize_query;
        using dependencies = rpnx::typelist< argument_initialize_by_class_conversion_query, argument_initialize_by_intrinsic_query, argument_initialize_by_template_query, bindable_query, ensig_argument_initialize_query >;
    };

    rpnx::querygraph::coroutine< ensig_argument_initialize_spec > ensig_argument_initialize_impl(argument_init_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ENSIG_ARGUMENT_INITIALIZE_SPEC_HEADER_GUARD
