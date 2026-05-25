// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/bindable_spec.hpp>
#include <quxlang/queries/specs/builtin_assignment_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_copy_ctor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_datatype_compare_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_default_ctor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_dtor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_move_ctor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_swap_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_template_instanciation_spec.hpp>
#include <quxlang/queries/specs/builtin_vm_procedure3_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_2(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< bindable_spec >(bindable_impl);
    graph.register_handler_function< builtin_assignment_vm_procedure3_spec >(builtin_assignment_vm_procedure3_impl);
    graph.register_handler_function< builtin_copy_ctor_vm_procedure3_spec >(builtin_copy_ctor_vm_procedure3_impl);
    graph.register_handler_function< builtin_datatype_compare_vm_procedure3_spec >(builtin_datatype_compare_vm_procedure3_impl);
    graph.register_handler_function< builtin_default_ctor_vm_procedure3_spec >(builtin_default_ctor_vm_procedure3_impl);
    graph.register_handler_function< builtin_dtor_vm_procedure3_spec >(builtin_dtor_vm_procedure3_impl);
    graph.register_handler_function< builtin_move_ctor_vm_procedure3_spec >(builtin_move_ctor_vm_procedure3_impl);
    graph.register_handler_function< builtin_swap_vm_procedure3_spec >(builtin_swap_vm_procedure3_impl);
    graph.register_handler_function< builtin_template_instanciation_spec >(builtin_template_instanciation_impl);
    graph.register_handler_function< builtin_vm_procedure3_spec >(builtin_vm_procedure3_impl);
}
