// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/source_file_id_spec.hpp>
#include <quxlang/queries/specs/source_file_index_spec.hpp>
#include <quxlang/queries/specs/source_file_name_spec.hpp>
#include <quxlang/queries/specs/static_test_vmir_spec.hpp>
#include <quxlang/queries/specs/string_static_value_spec.hpp>
#include <quxlang/queries/specs/symboid_spec.hpp>
#include <quxlang/queries/specs/symboid_subdeclaroids_spec.hpp>
#include <quxlang/queries/specs/symbol_tempars_spec.hpp>
#include <quxlang/queries/specs/symbol_type_spec.hpp>
#include <quxlang/queries/specs/template_builtin_spec.hpp>
#include <quxlang/queries/specs/template_instanciation_spec.hpp>
#include <quxlang/queries/specs/temploid_formal_ensig_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_7(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< source_file_id_spec >(source_file_id_impl);
    graph.register_handler_function< source_file_index_spec >(source_file_index_impl);
    graph.register_handler_function< source_file_name_spec >(source_file_name_impl);
    graph.register_handler_function< static_test_vmir_spec >(static_test_vmir_impl);
    graph.register_handler_function< string_static_value_spec >(string_static_value_impl);
    graph.register_handler_function< symboid_spec >(symboid_impl);
    graph.register_handler_function< symboid_subdeclaroids_spec >(symboid_subdeclaroids_impl);
    graph.register_handler_function< symbol_tempars_spec >(symbol_tempars_impl);
    graph.register_handler_function< symbol_type_spec >(symbol_type_impl);
    graph.register_handler_function< template_builtin_spec >(template_builtin_impl);
    graph.register_handler_function< template_instanciation_spec >(template_instanciation_impl);
    graph.register_handler_function< temploid_formal_ensig_spec >(temploid_formal_ensig_impl);
}
