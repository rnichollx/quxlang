// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/source_file_id_spec.hpp>
#include <quxlang/queries/specs/source_file_index_spec.hpp>
#include <quxlang/queries/specs/source_file_name_spec.hpp>
#include <quxlang/queries/specs/static_test_vmir_spec.hpp>
#include <quxlang/queries/specs/string_static_value_spec.hpp>
#include <quxlang/queries/specs/numeric_static_value_spec.hpp>
#include <quxlang/queries/specs/symboid_spec.hpp>
#include <quxlang/queries/specs/unit_test_vmir_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_13(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< source_file_id_spec >(source_file_id_impl);
    graph.register_handler_function< source_file_index_spec >(source_file_index_impl);
    graph.register_handler_function< source_file_name_spec >(source_file_name_impl);
    graph.register_handler_function< static_test_vmir_spec >(static_test_vmir_impl);
    graph.register_handler_function< string_static_value_spec >(string_static_value_impl);
    graph.register_handler_function< numeric_static_value_spec >(numeric_static_value_impl);
    graph.register_handler_function< symboid_spec >(symboid_impl);
    graph.register_handler_function< unit_test_vmir_spec >(unit_test_vmir_impl);
}
