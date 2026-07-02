// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/list_builtin_constructors_spec.hpp>
#include <quxlang/queries/specs/list_static_tests_spec.hpp>
#include <quxlang/queries/specs/list_unit_tests_spec.hpp>
#include <quxlang/queries/specs/list_user_functum_formal_paratypes_spec.hpp>
#include <quxlang/queries/specs/lookup_spec.hpp>
#include <quxlang/queries/specs/module_ast_spec.hpp>
#include <quxlang/queries/specs/module_options_map_spec.hpp>
#include <quxlang/queries/specs/module_source_name_spec.hpp>
#include <quxlang/queries/specs/test_is_enabled_for_static_testing_spec.hpp>
#include <quxlang/queries/specs/test_is_enabled_for_unit_testing_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_11(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< list_builtin_constructors_spec >(list_builtin_constructors_impl);
    graph.register_handler_function< list_static_tests_spec >(list_static_tests_impl);
    graph.register_handler_function< list_unit_tests_spec >(list_unit_tests_impl);
    graph.register_handler_function< list_user_functum_formal_paratypes_spec >(list_user_functum_formal_paratypes_impl);
    graph.register_handler_function< lookup_spec >(lookup_impl);
    graph.register_handler_function< module_ast_spec >(module_ast_impl);
    graph.register_handler_function< module_options_map_spec >(module_options_map_impl);
    graph.register_handler_function< module_source_name_spec >(module_source_name_impl);
    graph.register_handler_function< test_is_enabled_for_static_testing_spec >(test_is_enabled_for_static_testing_impl);
    graph.register_handler_function< test_is_enabled_for_unit_testing_spec >(test_is_enabled_for_unit_testing_impl);
}
