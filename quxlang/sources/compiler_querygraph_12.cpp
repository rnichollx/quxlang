// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/instanciation_spec.hpp>
#include <quxlang/queries/specs/instanciation_concrete_params_spec.hpp>
#include <quxlang/queries/specs/instanciation_tempar_map_spec.hpp>
#include <quxlang/queries/specs/interpret_bool_spec.hpp>
#include <quxlang/queries/specs/interpret_value_spec.hpp>
#include <quxlang/queries/specs/interface_defaultable_spec.hpp>
#include <quxlang/queries/specs/interface_slot_list_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_10(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< instanciation_spec >(instanciation_impl);
    graph.register_handler_function< instanciation_concrete_params_spec >(instanciation_concrete_params_impl);
    graph.register_handler_function< instanciation_tempar_map_spec >(instanciation_tempar_map_impl);
    graph.register_handler_function< interpret_bool_spec >(interpret_bool_impl);
    graph.register_handler_function< interpret_value_spec >(interpret_value_impl);
    graph.register_handler_function< interface_defaultable_spec >(interface_defaultable_impl);
    graph.register_handler_function< interface_slot_list_spec >(interface_slot_list_impl);
}
