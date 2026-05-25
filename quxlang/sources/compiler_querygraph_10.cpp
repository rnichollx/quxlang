// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/functum_builtins_spec.hpp>
#include <quxlang/queries/specs/functum_exists_and_is_callable_with_spec.hpp>
#include <quxlang/queries/specs/functum_initialize_spec.hpp>
#include <quxlang/queries/specs/functum_list_user_ensig_declarations_spec.hpp>
#include <quxlang/queries/specs/functum_list_user_overload_declarations_spec.hpp>
#include <quxlang/queries/specs/functum_map_user_formal_ensigs_spec.hpp>
#include <quxlang/queries/specs/functum_overloads_spec.hpp>
#include <quxlang/queries/specs/functum_select_function_spec.hpp>
#include <quxlang/queries/specs/functum_user_overloads_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_8(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< functum_builtins_spec >(functum_builtins_impl);
    graph.register_handler_function< functum_exists_and_is_callable_with_spec >(functum_exists_and_is_callable_with_impl);
    graph.register_handler_function< functum_initialize_spec >(functum_initialize_impl);
    graph.register_handler_function< functum_list_user_ensig_declarations_spec >(functum_list_user_ensig_declarations_impl);
    graph.register_handler_function< functum_list_user_overload_declarations_spec >(functum_list_user_overload_declarations_impl);
    graph.register_handler_function< functum_map_user_formal_ensigs_spec >(functum_map_user_formal_ensigs_impl);
    graph.register_handler_function< functum_overloads_spec >(functum_overloads_impl);
    graph.register_handler_function< functum_select_function_spec >(functum_select_function_impl);
    graph.register_handler_function< functum_user_overloads_spec >(functum_user_overloads_impl);
}
