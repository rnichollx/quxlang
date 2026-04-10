// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_BINDABLE_BY_REFERENCE_OBJECTIZATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_BINDABLE_BY_REFERENCE_OBJECTIZATION_SPEC_HEADER_GUARD

#include <quxlang/queries/bindable_by_reference_objectization.hpp>
#include <quxlang/queries/bindable_by_reference_requalification.hpp>
#include <quxlang/queries/functum_initialize.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using bindable_by_reference_objectization_spec = rpnx::querygraph::query_handler_spec< bindable_by_reference_objectization_query, rpnx::typelist< bindable_by_reference_requalification_query, functum_initialize_query > >;

    rpnx::querygraph::coroutine< bindable_by_reference_objectization_spec > bindable_by_reference_objectization_impl(implicitly_convertible_to_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_BINDABLE_BY_REFERENCE_OBJECTIZATION_SPEC_HEADER_GUARD
