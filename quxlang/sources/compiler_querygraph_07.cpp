// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/convertible_by_call_spec.hpp>
#include <quxlang/queries/specs/declaroids_spec.hpp>
#include <quxlang/queries/specs/ensig_argument_initialize_spec.hpp>
#include <quxlang/queries/specs/ensig_initialize_spec.hpp>
#include <quxlang/queries/specs/ensig_tempars_spec.hpp>
#include <quxlang/queries/specs/enum_info_spec.hpp>
#include <quxlang/queries/specs/exists_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_5(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< convertible_by_call_spec >(convertible_by_call_impl);
    graph.register_handler_function< declaroids_spec >(declaroids_impl);
    graph.register_handler_function< ensig_argument_initialize_spec >(ensig_argument_initialize_impl);
    graph.register_handler_function< ensig_initialize_spec >(ensig_initialize_impl);
    graph.register_handler_function< ensig_tempars_spec >(ensig_tempars_impl);
    graph.register_handler_function< enum_info_spec >(enum_info_impl);
    graph.register_handler_function< exists_spec >(exists_impl);
}
