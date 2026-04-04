// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_BINDABLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_BINDABLE_SPEC_HEADER_GUARD

#include <quxlang/queries/bindable.hpp>
#include <quxlang/queries/bindable_by_reference_objectization.hpp>
#include <quxlang/queries/bindable_by_reference_requalification.hpp>
#include <quxlang/queries/bindable_by_temporary_materialization.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using bindable_spec = rpnx::query_handler_spec< bindable_query, rpnx::typelist< bindable_by_reference_objectization_query, bindable_by_reference_requalification_query, bindable_by_temporary_materialization_query > >;

    rpnx::querygraph::coroutine< bindable_spec > bindable_impl(implicitly_convertible_to_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_BINDABLE_SPEC_HEADER_GUARD
