// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/user_move_ctor_exists_spec.hpp>
#include <quxlang/queries/specs/user_serialize_exists_spec.hpp>
#include <quxlang/queries/specs/user_swap_exists_spec.hpp>
#include <quxlang/queries/specs/user_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/variable_type_spec.hpp>
#include <quxlang/queries/specs/vm_procedure3_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_18(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< user_move_ctor_exists_spec >(user_move_ctor_exists_impl);
    graph.register_handler_function< user_serialize_exists_spec >(user_serialize_exists_impl);
    graph.register_handler_function< user_swap_exists_spec >(user_swap_exists_impl);
    graph.register_handler_function< user_vm_procedure3_spec >(user_vm_procedure3_impl);
    graph.register_handler_function< variable_type_spec >(variable_type_impl);
    graph.register_handler_function< vm_procedure3_spec >(vm_procedure3_impl);
}
