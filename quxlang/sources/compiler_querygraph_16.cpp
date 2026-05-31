// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/symboid_subdeclaroids_spec.hpp>
#include <quxlang/queries/specs/symbol_tempars_spec.hpp>
#include <quxlang/queries/specs/symbol_type_spec.hpp>
#include <quxlang/queries/specs/output_binaries_spec.hpp>
#include <quxlang/queries/specs/output_binary_spec.hpp>
#include <quxlang/queries/specs/output_llvm_input_spec.hpp>
#include <quxlang/queries/specs/output_list_spec.hpp>
#include <quxlang/queries/specs/output_optimized_llvm_spec.hpp>
#include <quxlang/queries/specs/output_unoptimized_llvm_spec.hpp>
#include <quxlang/queries/specs/template_builtin_spec.hpp>
#include <quxlang/queries/specs/template_instanciation_spec.hpp>
#include <quxlang/queries/specs/temploid_formal_ensig_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_14(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< symboid_subdeclaroids_spec >(symboid_subdeclaroids_impl);
    graph.register_handler_function< symbol_tempars_spec >(symbol_tempars_impl);
    graph.register_handler_function< symbol_type_spec >(symbol_type_impl);
    graph.register_handler_function< output_binaries_spec >(output_binaries_impl);
    graph.register_handler_function< output_binary_spec >(output_binary_impl);
    graph.register_handler_function< output_llvm_input_spec >(output_llvm_input_impl);
    graph.register_handler_function< output_list_spec >(output_list_impl);
    graph.register_handler_function< output_optimized_llvm_spec >(output_optimized_llvm_impl);
    graph.register_handler_function< output_unoptimized_llvm_spec >(output_unoptimized_llvm_impl);
    graph.register_handler_function< template_builtin_spec >(template_builtin_impl);
    graph.register_handler_function< template_instanciation_spec >(template_instanciation_impl);
    graph.register_handler_function< temploid_formal_ensig_spec >(temploid_formal_ensig_impl);
}
