// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/module_sources_spec.hpp>
#include <quxlang/queries/specs/parse_file_spec.hpp>
#include <quxlang/queries/specs/procedure_linksymbol_spec.hpp>
#include <quxlang/queries/specs/run_static_test_spec.hpp>
#include <quxlang/queries/specs/run_static_tests_spec.hpp>
#include <quxlang/queries/specs/serialoid_static_value_spec.hpp>
#include <quxlang/queries/specs/sintpointer_type_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_12(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< module_sources_spec >(module_sources_impl);
    graph.register_handler_function< parse_file_spec >(parse_file_impl);
    graph.register_handler_function< procedure_linksymbol_spec >(procedure_linksymbol_impl);
    graph.register_handler_function< run_static_test_spec >(run_static_test_impl);
    graph.register_handler_function< run_static_tests_spec >(run_static_tests_impl);
    graph.register_handler_function< serialoid_static_value_spec >(serialoid_static_value_impl);
    graph.register_handler_function< sintpointer_type_spec >(sintpointer_type_impl);
}
