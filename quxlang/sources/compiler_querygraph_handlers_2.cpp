// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/class_builtin_spec.hpp>
#include <quxlang/queries/specs/class_default_ctor_spec.hpp>
#include <quxlang/queries/specs/class_default_dtor_spec.hpp>
#include <quxlang/queries/specs/class_field_declaration_list_spec.hpp>
#include <quxlang/queries/specs/class_field_list_spec.hpp>
#include <quxlang/queries/specs/class_layout_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_assignment_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_copy_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_default_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_default_dtor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_move_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_swap_spec.hpp>
#include <quxlang/queries/specs/class_tags_spec.hpp>
#include <quxlang/queries/specs/class_trivially_constructible_spec.hpp>
#include <quxlang/queries/specs/class_trivially_destructible_spec.hpp>
#include <quxlang/queries/specs/constexpr_bool_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_string_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_v3_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_v3_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_spec.hpp>
#include <quxlang/queries/specs/constexpr_u64_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_2(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< class_builtin_spec >(class_builtin_impl);
    graph.register_handler_function< class_default_ctor_spec >(class_default_ctor_impl);
    graph.register_handler_function< class_default_dtor_spec >(class_default_dtor_impl);
    graph.register_handler_function< class_field_declaration_list_spec >(class_field_declaration_list_impl);
    graph.register_handler_function< class_field_list_spec >(class_field_list_impl);
    graph.register_handler_function< class_layout_spec >(class_layout_impl);
    graph.register_handler_function< class_requires_gen_assignment_spec >(class_requires_gen_assignment_impl);
    graph.register_handler_function< class_requires_gen_copy_ctor_spec >(class_requires_gen_copy_ctor_impl);
    graph.register_handler_function< class_requires_gen_default_ctor_spec >(class_requires_gen_default_ctor_impl);
    graph.register_handler_function< class_requires_gen_default_dtor_spec >(class_requires_gen_default_dtor_impl);
    graph.register_handler_function< class_requires_gen_move_ctor_spec >(class_requires_gen_move_ctor_impl);
    graph.register_handler_function< class_requires_gen_swap_spec >(class_requires_gen_swap_impl);
    graph.register_handler_function< class_tags_spec >(class_tags_impl);
    graph.register_handler_function< class_trivially_constructible_spec >(class_trivially_constructible_impl);
    graph.register_handler_function< class_trivially_destructible_spec >(class_trivially_destructible_impl);
    graph.register_handler_function< constexpr_bool_spec >(constexpr_bool_impl);
    graph.register_handler_function< constexpr_eval_antestatal_spec >(constexpr_eval_antestatal_impl);
    graph.register_handler_function< constexpr_eval_string_spec >(constexpr_eval_string_impl);
    graph.register_handler_function< constexpr_eval_v3_spec >(constexpr_eval_v3_impl);
    graph.register_handler_function< constexpr_eval_spec >(constexpr_eval_impl);
    graph.register_handler_function< constexpr_routine_antestatal_spec >(constexpr_routine_antestatal_impl);
    graph.register_handler_function< constexpr_routine_v3_spec >(constexpr_routine_v3_impl);
    graph.register_handler_function< constexpr_routine_spec >(constexpr_routine_impl);
    graph.register_handler_function< constexpr_u64_spec >(constexpr_u64_impl);
}
