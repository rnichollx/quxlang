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

auto quxlang::detail::register_compiler_querygraph_handlers_3(compiler_querygraph& querygraph) -> void
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
}
