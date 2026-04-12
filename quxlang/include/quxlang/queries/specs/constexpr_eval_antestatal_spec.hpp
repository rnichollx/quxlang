// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_ANTESTATAL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_ANTESTATAL_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/class_layout.hpp>
#include <quxlang/queries/constexpr_eval_antestatal.hpp>
#include <quxlang/queries/constexpr_routine_antestatal.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using constexpr_eval_antestatal_spec = rpnx::querygraph::query_handler_spec<
        constexpr_eval_antestatal_query,
        rpnx::typelist< antestatal_static_value_query, class_layout_query, constexpr_routine_antestatal_query, global_is_antestatal_static_query, variable_type_query, vm_procedure3_query > >;

    rpnx::querygraph::coroutine< constexpr_eval_antestatal_spec > constexpr_eval_antestatal_impl(constexpr_input2 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_ANTESTATAL_SPEC_HEADER_GUARD
