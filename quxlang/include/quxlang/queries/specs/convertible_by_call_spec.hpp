// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONVERTIBLE_BY_CALL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONVERTIBLE_BY_CALL_SPEC_HEADER_GUARD

#include <quxlang/queries/convertible_by_call.hpp>
#include <quxlang/queries/argument_initialize_by_class_conversion.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using convertible_by_call_spec = rpnx::querygraph::query_handler_spec< convertible_by_call_query, rpnx::typelist< argument_initialize_by_class_conversion_query > >;

    rpnx::querygraph::coroutine< convertible_by_call_spec > convertible_by_call_impl(implicitly_convertible_to_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONVERTIBLE_BY_CALL_SPEC_HEADER_GUARD
