// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/type_is_serialoid_spec.hpp>
#include <quxlang/queries/specs/type_is_stringlike_spec.hpp>
#include <quxlang/queries/specs/type_is_trivially_default_constructible_spec.hpp>
#include <quxlang/queries/specs/type_is_trivially_relocatable_spec.hpp>
#include <quxlang/queries/specs/class_placement_info_spec.hpp>
#include <quxlang/queries/specs/type_should_autogen_deserialize_spec.hpp>
#include <quxlang/queries/specs/type_should_autogen_serialize_spec.hpp>
#include <quxlang/queries/specs/uintpointer_type_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_16(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< type_is_serialoid_spec >(type_is_serialoid_impl);
    graph.register_handler_function< type_is_stringlike_spec >(type_is_stringlike_impl);
    graph.register_handler_function< type_is_trivially_default_constructible_spec >(type_is_trivially_default_constructible_impl);
    graph.register_handler_function< type_is_trivially_relocatable_spec >(type_is_trivially_relocatable_impl);
    graph.register_handler_function< class_placement_info_spec >(class_placement_info_impl);
    graph.register_handler_function< type_should_autogen_deserialize_spec >(type_should_autogen_deserialize_impl);
    graph.register_handler_function< type_should_autogen_serialize_spec >(type_should_autogen_serialize_impl);
    graph.register_handler_function< uintpointer_type_spec >(uintpointer_type_impl);
}
