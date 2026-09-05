// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/class_type_spec.hpp>
#include <quxlang/queries/specs/cortado_output_binary_artifact_spec.hpp>
#include <quxlang/queries/specs/llvm_compilation_unit_identities_spec.hpp>
#include <quxlang/queries/specs/llvm_compiler_builtin_manifest_spec.hpp>
#include <quxlang/queries/specs/llvm_function_module_spec.hpp>
#include <quxlang/queries/specs/llvm_output_binary_artifact_spec.hpp>
#include <quxlang/queries/specs/llvm_output_component_identities_spec.hpp>
#include <quxlang/queries/specs/llvm_post_codegen_spec.hpp>
#include <quxlang/queries/specs/llvm_postoptimize_spec.hpp>
#include <quxlang/queries/specs/llvm_preoptimize_spec.hpp>
#include <quxlang/queries/specs/output_binaries_information_spec.hpp>
#include <quxlang/queries/specs/output_binary_artifact_spec.hpp>
#include <quxlang/queries/specs/output_binary_artifacts_spec.hpp>
#include <quxlang/queries/specs/output_binary_information_spec.hpp>
#include <quxlang/queries/specs/output_build_settings_spec.hpp>
#include <quxlang/queries/specs/output_cortado_backend_options_spec.hpp>
#include <quxlang/queries/specs/output_cortado_input_spec.hpp>
#include <quxlang/queries/specs/output_list_spec.hpp>
#include <quxlang/queries/specs/output_llvm_backend_options_spec.hpp>
#include <quxlang/queries/specs/output_llvm_catalog_spec.hpp>
#include <quxlang/queries/specs/output_llvm_type_ordinals_spec.hpp>
#include <quxlang/queries/specs/output_steppings_spec.hpp>
#include <quxlang/queries/specs/symboid_subdeclaroids_spec.hpp>
#include <quxlang/queries/specs/symbol_tempars_spec.hpp>
#include <quxlang/queries/specs/symbol_type_spec.hpp>
#include <quxlang/queries/specs/target_backend_spec.hpp>
#include <quxlang/queries/specs/target_cortado_backend_options_spec.hpp>
#include <quxlang/queries/specs/target_llvm_backend_options_spec.hpp>
#include <quxlang/queries/specs/template_builtin_spec.hpp>
#include <quxlang/queries/specs/template_instanciation_spec.hpp>
#include <quxlang/queries/specs/temploid_formal_ensig_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_14(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< symboid_subdeclaroids_spec >(symboid_subdeclaroids_impl);
    graph.register_handler_function< symbol_tempars_spec >(symbol_tempars_impl);
    graph.register_handler_function< symbol_type_spec >(symbol_type_impl);
    graph.register_handler_function< class_type_spec >(class_type_impl);
    graph.register_handler_function< cortado_output_binary_artifact_spec >(cortado_output_binary_artifact_impl);
    graph.register_handler_function< llvm_compiler_builtin_manifest_spec >(llvm_compiler_builtin_manifest_impl);
    graph.register_handler_function< llvm_output_component_identities_spec >(llvm_output_component_identities_impl);
    graph.register_handler_function< llvm_post_codegen_spec >(llvm_post_codegen_impl);
    graph.register_handler_function< llvm_postoptimize_spec >(llvm_postoptimize_impl);
    graph.register_handler_function< llvm_preoptimize_spec >(llvm_preoptimize_impl);
    graph.register_handler_function< llvm_function_module_spec >(llvm_function_module_impl);
    graph.register_handler_function< llvm_output_binary_artifact_spec >(llvm_output_binary_artifact_impl);
    graph.register_handler_function< output_binaries_information_spec >(output_binaries_information_impl);
    graph.register_handler_function< output_binary_artifact_spec >(output_binary_artifact_impl);
    graph.register_handler_function< output_binary_artifacts_spec >(output_binary_artifacts_impl);
    graph.register_handler_function< output_binary_information_spec >(output_binary_information_impl);
    graph.register_handler_function< output_llvm_backend_options_spec >(output_llvm_backend_options_impl);
    graph.register_handler_function< output_cortado_backend_options_spec >(output_cortado_backend_options_impl);
    graph.register_handler_function< output_cortado_input_spec >(output_cortado_input_impl);
    graph.register_handler_function< output_llvm_catalog_spec >(output_llvm_catalog_impl);
    graph.register_handler_function< llvm_compilation_unit_identities_spec >(llvm_compilation_unit_identities_impl);
    graph.register_handler_function< output_llvm_type_ordinals_spec >(output_llvm_type_ordinals_impl);
    graph.register_handler_function< output_list_spec >(output_list_impl);
    graph.register_handler_function< target_backend_spec >(target_backend_impl);
    graph.register_handler_function< target_llvm_backend_options_spec >(target_llvm_backend_options_impl);
    graph.register_handler_function< target_cortado_backend_options_spec >(target_cortado_backend_options_impl);
    graph.register_handler_function< output_build_settings_spec >(output_build_settings_impl);
    graph.register_handler_function< output_steppings_spec >(output_steppings_impl);
    graph.register_handler_function< template_builtin_spec >(template_builtin_impl);
    graph.register_handler_function< template_instanciation_spec >(template_instanciation_impl);
    graph.register_handler_function< temploid_formal_ensig_spec >(temploid_formal_ensig_impl);
}
