// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/user_assignment_exists_spec.hpp>
#include <quxlang/queries/specs/user_copy_ctor_exists_spec.hpp>
#include <quxlang/queries/specs/user_default_ctor_exists_spec.hpp>
#include <quxlang/queries/specs/user_default_dtor_exists_spec.hpp>
#include <quxlang/queries/specs/user_deserialize_exists_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_17(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< user_assignment_exists_spec >(user_assignment_exists_impl);
    graph.register_handler_function< user_copy_ctor_exists_spec >(user_copy_ctor_exists_impl);
    graph.register_handler_function< user_default_ctor_exists_spec >(user_default_ctor_exists_impl);
    graph.register_handler_function< user_default_dtor_exists_spec >(user_default_dtor_exists_impl);
    graph.register_handler_function< user_deserialize_exists_spec >(user_deserialize_exists_impl);
}
