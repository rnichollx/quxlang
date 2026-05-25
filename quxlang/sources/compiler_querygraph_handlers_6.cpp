// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/list_builtin_constructors_spec.hpp>
#include <quxlang/queries/specs/list_static_tests_spec.hpp>
#include <quxlang/queries/specs/list_user_functum_formal_paratypes_spec.hpp>
#include <quxlang/queries/specs/lookup_spec.hpp>
#include <quxlang/queries/specs/module_ast_spec.hpp>
#include <quxlang/queries/specs/module_options_map_spec.hpp>
#include <quxlang/queries/specs/module_source_name_spec.hpp>
#include <quxlang/queries/specs/module_sources_spec.hpp>
#include <quxlang/queries/specs/parse_file_spec.hpp>
#include <quxlang/queries/specs/procedure_linksymbol_spec.hpp>
#include <quxlang/queries/specs/run_static_test_spec.hpp>
#include <quxlang/queries/specs/run_static_tests_spec.hpp>
#include <quxlang/queries/specs/serialoid_static_value_spec.hpp>
#include <quxlang/queries/specs/sintpointer_type_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_6(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< list_builtin_constructors_spec >(list_builtin_constructors_impl);
    graph.register_handler_function< list_static_tests_spec >(list_static_tests_impl);
    graph.register_handler_function< list_user_functum_formal_paratypes_spec >(list_user_functum_formal_paratypes_impl);
    graph.register_handler_function< lookup_spec >(lookup_impl);
    graph.register_handler_function< module_ast_spec >(module_ast_impl);
    graph.register_handler_function< module_options_map_spec >(module_options_map_impl);
    graph.register_handler_function< module_source_name_spec >(module_source_name_impl);
    graph.register_handler_function< module_sources_spec >(module_sources_impl);
    graph.register_handler_function< parse_file_spec >(parse_file_impl);
    graph.register_handler_function< procedure_linksymbol_spec >(procedure_linksymbol_impl);
    graph.register_handler_function< run_static_test_spec >(run_static_test_impl);
    graph.register_handler_function< run_static_tests_spec >(run_static_tests_impl);
    graph.register_handler_function< serialoid_static_value_spec >(serialoid_static_value_impl);
    graph.register_handler_function< sintpointer_type_spec >(sintpointer_type_impl);
}
