// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_eval.hpp>
#include <quxlang/queries/constexpr_routine.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using constexpr_eval_spec = rpnx::query_handler_spec< constexpr_eval_query, rpnx::typelist< constexpr_routine_query, vm_procedure3_query > >;

    rpnx::querygraph::coroutine< constexpr_eval_spec > constexpr_eval_impl(constexpr_input2 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD
