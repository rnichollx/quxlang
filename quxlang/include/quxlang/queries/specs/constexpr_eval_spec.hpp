// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/class_layout.hpp>
#include <quxlang/queries/constexpr_eval.hpp>
#include <quxlang/queries/constexpr_routine.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using constexpr_eval_spec = rpnx::querygraph::query_handler_spec<
        constexpr_eval_query,
        rpnx::typelist< antestatal_static_value_query, class_layout_query, constexpr_routine_query, global_is_antestatal_static_query, source_bundle_query, source_file_index_query, variable_type_query, vm_procedure3_query > >;

    rpnx::querygraph::coroutine< constexpr_eval_spec > constexpr_eval_impl(constexpr_input2 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD
