// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/templex_builtins_spec.hpp>
#include <quxlang/queries/specs/templex_builtin_templates_spec.hpp>
#include <quxlang/queries/specs/templex_initialize_spec.hpp>
#include <quxlang/queries/specs/templex_select_template_spec.hpp>
#include <quxlang/queries/specs/type_is_antestatal_spec.hpp>
#include <quxlang/queries/specs/type_is_implicitly_datatype_spec.hpp>
#include <quxlang/queries/specs/type_is_serialoid_spec.hpp>
#include <quxlang/queries/specs/type_is_stringlike_spec.hpp>
#include <quxlang/queries/specs/type_placement_info_spec.hpp>
#include <quxlang/queries/specs/type_should_autogen_deserialize_spec.hpp>
#include <quxlang/queries/specs/type_should_autogen_serialize_spec.hpp>
#include <quxlang/queries/specs/uintpointer_type_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_8(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< templex_builtins_spec >(templex_builtins_impl);
    graph.register_handler_function< templex_builtin_templates_spec >(templex_builtin_templates_impl);
    graph.register_handler_function< templex_initialize_spec >(templex_initialize_impl);
    graph.register_handler_function< templex_select_template_spec >(templex_select_template_impl);
    graph.register_handler_function< type_is_antestatal_spec >(type_is_antestatal_impl);
    graph.register_handler_function< type_is_serialoid_spec >(type_is_serialoid_impl);
    graph.register_handler_function< type_is_stringlike_spec >(type_is_stringlike_impl);
    graph.register_handler_function< type_is_implicitly_datatype_spec >(type_is_implicitly_datatype_impl);
    graph.register_handler_function< type_placement_info_spec >(type_placement_info_impl);
    graph.register_handler_function< type_should_autogen_deserialize_spec >(type_should_autogen_deserialize_impl);
    graph.register_handler_function< type_should_autogen_serialize_spec >(type_should_autogen_serialize_impl);
    graph.register_handler_function< uintpointer_type_spec >(uintpointer_type_impl);
}
