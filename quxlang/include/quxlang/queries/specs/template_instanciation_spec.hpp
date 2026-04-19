// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLATE_INSTANCIATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLATE_INSTANCIATION_SPEC_HEADER_GUARD

#include <quxlang/queries/template_instanciation.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using template_instanciation_spec = rpnx::querygraph::query_handler_spec< template_instanciation_query, rpnx::typelist< constexpr_eval_v3_query, lookup_query, symboid_query, symbol_type_query > >;

    rpnx::querygraph::coroutine< template_instanciation_spec > template_instanciation_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLATE_INSTANCIATION_SPEC_HEADER_GUARD
