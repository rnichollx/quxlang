// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/function_builtin_spec.hpp>
#include <quxlang/queries/specs/function_declaration_spec.hpp>
#include <quxlang/queries/specs/function_ensig_init_with_spec.hpp>
#include <quxlang/queries/specs/function_instanciation_spec.hpp>
#include <quxlang/queries/specs/function_pack_info_spec.hpp>
#include <quxlang/queries/specs/function_param_names_spec.hpp>
#include <quxlang/queries/specs/function_positional_parameter_names_spec.hpp>
#include <quxlang/queries/specs/function_primitive_spec.hpp>
#include <quxlang/queries/specs/functum_builtin_overloads_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_7(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< function_builtin_spec >(function_builtin_impl);
    graph.register_handler_function< function_declaration_spec >(function_declaration_impl);
    graph.register_handler_function< function_ensig_init_with_spec >(function_ensig_init_with_impl);
    graph.register_handler_function< function_instanciation_spec >(function_instanciation_impl);
    graph.register_handler_function< function_pack_info_spec >(function_pack_info_impl);
    graph.register_handler_function< function_param_names_spec >(function_param_names_impl);
    graph.register_handler_function< function_positional_parameter_names_spec >(function_positional_parameter_names_impl);
    graph.register_handler_function< function_primitive_spec >(function_primitive_impl);
    graph.register_handler_function< functum_builtin_overloads_spec >(functum_builtin_overloads_impl);
}
