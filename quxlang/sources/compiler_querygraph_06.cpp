// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/class_requires_gen_swap_spec.hpp>
#include <quxlang/queries/specs/class_tags_spec.hpp>
#include <quxlang/queries/specs/class_trivially_constructible_spec.hpp>
#include <quxlang/queries/specs/class_trivially_destructible_spec.hpp>
#include <quxlang/queries/specs/constexpr_bool_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_string_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_numeric_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_v3_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_v3_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_spec.hpp>
#include <quxlang/queries/specs/constexpr_u64_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_4(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< class_requires_gen_swap_spec >(class_requires_gen_swap_impl);
    graph.register_handler_function< class_tags_spec >(class_tags_impl);
    graph.register_handler_function< class_trivially_constructible_spec >(class_trivially_constructible_impl);
    graph.register_handler_function< class_trivially_destructible_spec >(class_trivially_destructible_impl);
    graph.register_handler_function< constexpr_bool_spec >(constexpr_bool_impl);
    graph.register_handler_function< constexpr_eval_antestatal_spec >(constexpr_eval_antestatal_impl);
    graph.register_handler_function< constexpr_eval_string_spec >(constexpr_eval_string_impl);
    graph.register_handler_function< constexpr_eval_numeric_spec >(constexpr_eval_numeric_impl);
    graph.register_handler_function< constexpr_eval_v3_spec >(constexpr_eval_v3_impl);
    graph.register_handler_function< constexpr_eval_spec >(constexpr_eval_impl);
    graph.register_handler_function< constexpr_routine_antestatal_spec >(constexpr_routine_antestatal_impl);
    graph.register_handler_function< constexpr_routine_v3_spec >(constexpr_routine_v3_impl);
    graph.register_handler_function< constexpr_routine_spec >(constexpr_routine_impl);
    graph.register_handler_function< constexpr_u64_spec >(constexpr_u64_impl);
}
