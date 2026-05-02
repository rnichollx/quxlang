// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLEX_SELECT_TEMPLATE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLEX_SELECT_TEMPLATE_SPEC_HEADER_GUARD

#include <new>

#include <rpnx/querygraph/querygraph.hpp>

#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/specs/symboid_spec.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/templex_builtins.hpp>
#include <quxlang/queries/templex_select_template.hpp>

namespace quxlang
{
    struct templex_select_template_spec
    {
        using query = templex_select_template_query;
        using dependencies = rpnx::typelist< constexpr_eval_v3_query, lookup_query, symboid_query, symbol_type_query, templex_builtins_query >;
    };

    rpnx::querygraph::coroutine< templex_select_template_spec > templex_select_template_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLEX_SELECT_TEMPLATE_SPEC_HEADER_GUARD
