// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_exists_and_is_callable_with.hpp>
#include <quxlang/queries/functum_initialize.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using functum_exists_and_is_callable_with_spec = rpnx::querygraph::query_handler_spec< functum_exists_and_is_callable_with_query, rpnx::typelist< functum_initialize_query > >;

    rpnx::querygraph::coroutine< functum_exists_and_is_callable_with_spec > functum_exists_and_is_callable_with_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_SPEC_HEADER_GUARD
