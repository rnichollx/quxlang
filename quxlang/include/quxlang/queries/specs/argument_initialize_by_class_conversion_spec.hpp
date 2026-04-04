// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ARGUMENT_INITIALIZE_BY_CLASS_CONVERSION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ARGUMENT_INITIALIZE_BY_CLASS_CONVERSION_SPEC_HEADER_GUARD

#include <quxlang/queries/argument_initialize_by_class_conversion.hpp>
#include <quxlang/queries/bindable.hpp>
#include <quxlang/queries/functum_initialize.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using argument_initialize_by_class_conversion_spec = rpnx::query_handler_spec< argument_initialize_by_class_conversion_query, rpnx::typelist< bindable_query, functum_initialize_query > >;

    rpnx::querygraph::coroutine< argument_initialize_by_class_conversion_spec > argument_initialize_by_class_conversion_impl(argument_init_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ARGUMENT_INITIALIZE_BY_CLASS_CONVERSION_SPEC_HEADER_GUARD
