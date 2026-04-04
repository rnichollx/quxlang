// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_BINDABLE_BY_REFERENCE_REQUALIFICATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_BINDABLE_BY_REFERENCE_REQUALIFICATION_SPEC_HEADER_GUARD

#include <quxlang/queries/bindable_by_reference_requalification.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using bindable_by_reference_requalification_spec = rpnx::query_handler_spec< bindable_by_reference_requalification_query, rpnx::typelist<  > >;

    rpnx::querygraph::coroutine< bindable_by_reference_requalification_spec > bindable_by_reference_requalification_impl(implicitly_convertible_to_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_BINDABLE_BY_REFERENCE_REQUALIFICATION_SPEC_HEADER_GUARD
