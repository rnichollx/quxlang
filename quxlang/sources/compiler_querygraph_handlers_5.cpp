// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/global_is_antestatal_static_spec.hpp>
#include <quxlang/queries/specs/global_is_serialoid_static_spec.hpp>
#include <quxlang/queries/specs/global_is_string_static_spec.hpp>
#include <quxlang/queries/specs/have_nontrivial_member_ctor_spec.hpp>
#include <quxlang/queries/specs/have_nontrivial_member_dtor_spec.hpp>
#include <quxlang/queries/specs/implementation_function_map_spec.hpp>
#include <quxlang/queries/specs/implementation_interface_type_spec.hpp>
#include <quxlang/queries/specs/implicitly_convertible_to_spec.hpp>
#include <quxlang/queries/specs/instanciation_spec.hpp>
#include <quxlang/queries/specs/instanciation_concrete_params_spec.hpp>
#include <quxlang/queries/specs/instanciation_tempar_map_spec.hpp>
#include <quxlang/queries/specs/interpret_bool_spec.hpp>
#include <quxlang/queries/specs/interpret_value_spec.hpp>
#include <quxlang/queries/specs/interface_defaultable_spec.hpp>
#include <quxlang/queries/specs/interface_slot_list_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_5(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< global_is_antestatal_static_spec >(global_is_antestatal_static_impl);
    graph.register_handler_function< global_is_serialoid_static_spec >(global_is_serialoid_static_impl);
    graph.register_handler_function< global_is_string_static_spec >(global_is_string_static_impl);
    graph.register_handler_function< have_nontrivial_member_ctor_spec >(have_nontrivial_member_ctor_impl);
    graph.register_handler_function< have_nontrivial_member_dtor_spec >(have_nontrivial_member_dtor_impl);
    graph.register_handler_function< implementation_function_map_spec >(implementation_function_map_impl);
    graph.register_handler_function< implementation_interface_type_spec >(implementation_interface_type_impl);
    graph.register_handler_function< implicitly_convertible_to_spec >(implicitly_convertible_to_impl);
    graph.register_handler_function< instanciation_spec >(instanciation_impl);
    graph.register_handler_function< instanciation_concrete_params_spec >(instanciation_concrete_params_impl);
    graph.register_handler_function< instanciation_tempar_map_spec >(instanciation_tempar_map_impl);
    graph.register_handler_function< interpret_bool_spec >(interpret_bool_impl);
    graph.register_handler_function< interpret_value_spec >(interpret_value_impl);
    graph.register_handler_function< interface_defaultable_spec >(interface_defaultable_impl);
    graph.register_handler_function< interface_slot_list_spec >(interface_slot_list_impl);
}
