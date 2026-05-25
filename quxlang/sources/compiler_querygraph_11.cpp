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

auto quxlang::detail::register_compiler_querygraph_handlers_9(compiler_querygraph& querygraph) -> void
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
}
