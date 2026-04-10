// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INSTANCIATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INSTANCIATION_SPEC_HEADER_GUARD

#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/function_instanciation.hpp>
#include <quxlang/queries/functum_initialize.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/template_instanciation.hpp>
#include <quxlang/queries/templex_initialize.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using instanciation_spec = rpnx::querygraph::query_handler_spec< instanciation_query, rpnx::typelist< function_instanciation_query, functum_initialize_query, symbol_type_query, template_instanciation_query, templex_initialize_query > >;

    rpnx::querygraph::coroutine< instanciation_spec > instanciation_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INSTANCIATION_SPEC_HEADER_GUARD
