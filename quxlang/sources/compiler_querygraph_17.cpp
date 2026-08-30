// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/templex_builtins_spec.hpp>
#include <quxlang/queries/specs/templex_builtin_templates_spec.hpp>
#include <quxlang/queries/specs/templex_initialize_spec.hpp>
#include <quxlang/queries/specs/templex_select_template_spec.hpp>
#include <quxlang/queries/specs/type_is_antestatal_spec.hpp>
#include <quxlang/queries/specs/type_is_implicitly_datatype_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_15(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< templex_builtins_spec >(templex_builtins_impl);
    graph.register_handler_function< templex_builtin_templates_spec >(templex_builtin_templates_impl);
    graph.register_handler_function< templex_initialize_spec >(templex_initialize_impl);
    graph.register_handler_function< templex_select_template_spec >(templex_select_template_impl);
    graph.register_handler_function< type_is_antestatal_spec >(type_is_antestatal_impl);
    graph.register_handler_function< type_is_implicitly_datatype_spec >(type_is_implicitly_datatype_impl);
}
