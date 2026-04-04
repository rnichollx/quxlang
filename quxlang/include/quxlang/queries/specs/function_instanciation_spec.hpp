// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_INSTANCIATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_INSTANCIATION_SPEC_HEADER_GUARD

#include <quxlang/queries/function_instanciation.hpp>
#include <quxlang/queries/function_ensig_init_with.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using function_instanciation_spec = rpnx::query_handler_spec< function_instanciation_query, rpnx::typelist< function_ensig_init_with_query, symbol_type_query > >;

    rpnx::querygraph::coroutine< function_instanciation_spec > function_instanciation_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_INSTANCIATION_SPEC_HEADER_GUARD
