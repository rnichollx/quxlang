// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_SELECT_FUNCTION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_SELECT_FUNCTION_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_select_function.hpp>
#include <quxlang/queries/argument_adaptation_is_better_fit.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/function_ensig_init_with.hpp>
#include <quxlang/queries/functum_overloads.hpp>
#include <quxlang/queries/instanciation_tempar_map.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functum_select_function_spec
    {
        using query = functum_select_function_query;
        using dependencies = rpnx::typelist< argument_adaptation_is_better_fit_query, constexpr_bool_query, function_ensig_init_with_query, functum_overloads_query, instanciation_tempar_map_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< functum_select_function_spec > functum_select_function_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_SELECT_FUNCTION_SPEC_HEADER_GUARD
