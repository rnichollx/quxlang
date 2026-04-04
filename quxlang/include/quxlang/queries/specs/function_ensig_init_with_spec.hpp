// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_ENSIG_INIT_WITH_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_ENSIG_INIT_WITH_SPEC_HEADER_GUARD

#include <quxlang/queries/function_ensig_init_with.hpp>
#include <quxlang/queries/ensig_argument_initialize.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using function_ensig_init_with_spec = rpnx::query_handler_spec< function_ensig_init_with_query, rpnx::typelist< ensig_argument_initialize_query > >;

    rpnx::querygraph::coroutine< function_ensig_init_with_spec > function_ensig_init_with_impl(ensig_initialization input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_ENSIG_INIT_WITH_SPEC_HEADER_GUARD
