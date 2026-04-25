// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/queries/specs/argument_adaptation_is_better_fit_spec.hpp>
#include <quxlang/queries/specs/argument_adaptation_rank_spec.hpp>
#include <quxlang/queries/specs/argument_initialize_by_class_conversion_spec.hpp>
#include <quxlang/queries/specs/argument_initialize_by_intrinsic_spec.hpp>
#include <quxlang/queries/specs/argument_initialize_by_template_spec.hpp>
#include <quxlang/queries/specs/antestatal_static_value_spec.hpp>
#include <quxlang/queries/specs/asm_procedure_from_symbol_spec.hpp>
#include <quxlang/queries/specs/bindable_by_reference_objectization_spec.hpp>
#include <quxlang/queries/specs/bindable_by_reference_requalification_spec.hpp>
#include <quxlang/queries/specs/bindable_by_temporary_materialization_spec.hpp>
#include <quxlang/queries/specs/bindable_spec.hpp>
#include <quxlang/queries/specs/builtin_assignment_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_copy_ctor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_datatype_compare_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_default_ctor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_dtor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_move_ctor_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_swap_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/builtin_template_instanciation_spec.hpp>
#include <quxlang/queries/specs/builtin_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/class_builtin_spec.hpp>
#include <quxlang/queries/specs/class_default_ctor_spec.hpp>
#include <quxlang/queries/specs/class_default_dtor_spec.hpp>
#include <quxlang/queries/specs/class_field_declaration_list_spec.hpp>
#include <quxlang/queries/specs/class_field_list_spec.hpp>
#include <quxlang/queries/specs/class_layout_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_assignment_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_copy_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_default_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_default_dtor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_move_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_swap_spec.hpp>
#include <quxlang/queries/specs/class_tags_spec.hpp>
#include <quxlang/queries/specs/class_trivially_constructible_spec.hpp>
#include <quxlang/queries/specs/class_trivially_destructible_spec.hpp>
#include <quxlang/queries/specs/constexpr_bool_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_string_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_v3_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_v3_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_spec.hpp>
#include <quxlang/queries/specs/constexpr_u64_spec.hpp>
#include <quxlang/queries/specs/convertible_by_call_spec.hpp>
#include <quxlang/queries/specs/declaroids_spec.hpp>
#include <quxlang/queries/specs/ensig_argument_initialize_spec.hpp>
#include <quxlang/queries/specs/ensig_initialize_spec.hpp>
#include <quxlang/queries/specs/ensig_tempars_spec.hpp>
#include <quxlang/queries/specs/exists_spec.hpp>
#include <quxlang/queries/specs/extern_linksymbol_spec.hpp>
#include <quxlang/queries/specs/functanoid_return_type_spec.hpp>
#include <quxlang/queries/specs/functanoid_sigtype_spec.hpp>
#include <quxlang/queries/specs/function_builtin_spec.hpp>
#include <quxlang/queries/specs/function_declaration_spec.hpp>
#include <quxlang/queries/specs/function_ensig_init_with_spec.hpp>
#include <quxlang/queries/specs/function_instanciation_spec.hpp>
#include <quxlang/queries/specs/function_pack_info_spec.hpp>
#include <quxlang/queries/specs/function_param_names_spec.hpp>
#include <quxlang/queries/specs/function_positional_parameter_names_spec.hpp>
#include <quxlang/queries/specs/function_primitive_spec.hpp>
#include <quxlang/queries/specs/functum_builtin_overloads_spec.hpp>
#include <quxlang/queries/specs/functum_builtins_spec.hpp>
#include <quxlang/queries/specs/functum_exists_and_is_callable_with_spec.hpp>
#include <quxlang/queries/specs/functum_initialize_spec.hpp>
#include <quxlang/queries/specs/functum_list_user_ensig_declarations_spec.hpp>
#include <quxlang/queries/specs/functum_list_user_overload_declarations_spec.hpp>
#include <quxlang/queries/specs/functum_map_user_formal_ensigs_spec.hpp>
#include <quxlang/queries/specs/functum_overloads_spec.hpp>
#include <quxlang/queries/specs/functum_select_function_spec.hpp>
#include <quxlang/queries/specs/functum_user_overloads_spec.hpp>
#include <quxlang/queries/specs/global_is_antestatal_static_spec.hpp>
#include <quxlang/queries/specs/global_is_serialoid_static_spec.hpp>
#include <quxlang/queries/specs/global_is_string_static_spec.hpp>
#include <quxlang/queries/specs/have_nontrivial_member_ctor_spec.hpp>
#include <quxlang/queries/specs/have_nontrivial_member_dtor_spec.hpp>
#include <quxlang/queries/specs/implicitly_convertible_to_spec.hpp>
#include <quxlang/queries/specs/instanciation_spec.hpp>
#include <quxlang/queries/specs/instanciation_tempar_map_spec.hpp>
#include <quxlang/queries/specs/interpret_bool_spec.hpp>
#include <quxlang/queries/specs/interpret_value_spec.hpp>
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
#include <quxlang/queries/specs/source_file_id_spec.hpp>
#include <quxlang/queries/specs/source_file_index_spec.hpp>
#include <quxlang/queries/specs/source_file_name_spec.hpp>
#include <quxlang/queries/specs/string_static_value_spec.hpp>
#include <quxlang/queries/specs/symboid_spec.hpp>
#include <quxlang/queries/specs/symboid_subdeclaroids_spec.hpp>
#include <quxlang/queries/specs/symbol_tempars_spec.hpp>
#include <quxlang/queries/specs/symbol_type_spec.hpp>
#include <quxlang/queries/specs/template_builtin_spec.hpp>
#include <quxlang/queries/specs/template_instanciation_spec.hpp>
#include <quxlang/queries/specs/templex_builtins_spec.hpp>
#include <quxlang/queries/specs/templex_builtin_templates_spec.hpp>
#include <quxlang/queries/specs/templex_initialize_spec.hpp>
#include <quxlang/queries/specs/templex_select_template_spec.hpp>
#include <quxlang/queries/specs/type_is_antestatal_spec.hpp>
#include <quxlang/queries/specs/type_is_serialoid_spec.hpp>
#include <quxlang/queries/specs/type_is_stringlike_spec.hpp>
#include <quxlang/queries/specs/type_is_implicitly_datatype_spec.hpp>
#include <quxlang/queries/specs/type_placement_info_spec.hpp>
#include <quxlang/queries/specs/type_should_autogen_deserialize_spec.hpp>
#include <quxlang/queries/specs/type_should_autogen_serialize_spec.hpp>
#include <quxlang/queries/specs/uintpointer_type_spec.hpp>
#include <quxlang/queries/specs/user_assignment_exists_spec.hpp>
#include <quxlang/queries/specs/user_copy_ctor_exists_spec.hpp>
#include <quxlang/queries/specs/user_default_ctor_exists_spec.hpp>
#include <quxlang/queries/specs/user_default_dtor_exists_spec.hpp>
#include <quxlang/queries/specs/user_deserialize_exists_spec.hpp>
#include <quxlang/queries/specs/user_move_ctor_exists_spec.hpp>
#include <quxlang/queries/specs/user_serialize_exists_spec.hpp>
#include <quxlang/queries/specs/user_swap_exists_spec.hpp>
#include <quxlang/queries/specs/user_vm_procedure3_spec.hpp>
#include <quxlang/queries/specs/variable_type_spec.hpp>
#include <quxlang/queries/specs/vm_procedure3_spec.hpp>

#include <quxlang/macros.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_option_strings_map.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/querygraph_traits.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/vmir2/assembler.hpp>

#include <fstream>
#include <format>
#include <iostream>
#include <map>
#include <string>
#include <utility>

auto rpnx::querygraph::debug_traits< quxlang::vmir2::functanoid_routine3 >::to_debug_string(quxlang::vmir2::functanoid_routine3 const& value) -> std::string
{
    quxlang::vmir2::assembler assembler(value);
    return assembler.to_string(value);
}

namespace
{
    auto write_marshaled_graph_dump(rpnx::querygraph::graph& graph, std::filesystem::path const& output_path) -> void
    {
        auto const marshaled_dump = graph.marshall();
        std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw std::runtime_error(std::format("Failed to open dump file for writing: {}", output_path.string()));
        }

        if (!marshaled_dump.empty())
        {
            out.write(reinterpret_cast< char const* >(marshaled_dump.data()), static_cast< std::streamsize >(marshaled_dump.size()));
        }
        if (!out)
        {
            throw std::runtime_error(std::format("Failed to write dump file: {}", output_path.string()));
        }
    }
} // namespace

quxlang::compiler_querygraph::compiler_querygraph(source_bundle const& bundle, std::string configured_target, output_info const& machine_info,
                                                  std::optional< std::filesystem::path > dump_output_path)
    : m_dump_output_path(std::move(dump_output_path))
{
    std::map< std::string, std::string > module_source_name_map;
    std::map< std::string, std::map< std::string, std::string > > module_option_strings_map;
    auto const& target_config = bundle.targets.at(configured_target);
    for (auto const& [logical_name, module_config] : target_config.module_configurations)
    {
        module_source_name_map.emplace(logical_name, module_config.source);
        module_option_strings_map.emplace(logical_name, module_config.option_values);
    }

    m_graph.register_handler_singleton< source_bundle_query >(bundle);
    m_graph.register_handler_singleton< machine_info_query >(machine_info);
    m_graph.register_handler_singleton< module_source_name_map_query >(std::move(module_source_name_map));
    m_graph.register_handler_singleton< module_option_strings_map_query >(std::move(module_option_strings_map));

    m_graph.register_handler_function< argument_adaptation_is_better_fit_spec >(argument_adaptation_is_better_fit_impl);
    m_graph.register_handler_function< argument_adaptation_rank_spec >(argument_adaptation_rank_impl);
    m_graph.register_handler_function< argument_initialize_by_class_conversion_spec >(argument_initialize_by_class_conversion_impl);
    m_graph.register_handler_function< argument_initialize_by_intrinsic_spec >(argument_initialize_by_intrinsic_impl);
    m_graph.register_handler_function< argument_initialize_by_template_spec >(argument_initialize_by_template_impl);
    m_graph.register_handler_function< antestatal_static_value_spec >(antestatal_static_value_impl);
    m_graph.register_handler_function< asm_procedure_from_symbol_spec >(asm_procedure_from_symbol_impl);
    m_graph.register_handler_function< bindable_spec >(bindable_impl);
    m_graph.register_handler_function< bindable_by_reference_objectization_spec >(bindable_by_reference_objectization_impl);
    m_graph.register_handler_function< bindable_by_reference_requalification_spec >(bindable_by_reference_requalification_impl);
    m_graph.register_handler_function< bindable_by_temporary_materialization_spec >(bindable_by_temporary_materialization_impl);
    m_graph.register_handler_function< builtin_assignment_vm_procedure3_spec >(builtin_assignment_vm_procedure3_impl);
    m_graph.register_handler_function< builtin_copy_ctor_vm_procedure3_spec >(builtin_copy_ctor_vm_procedure3_impl);
    m_graph.register_handler_function< builtin_datatype_compare_vm_procedure3_spec >(builtin_datatype_compare_vm_procedure3_impl);
    m_graph.register_handler_function< builtin_default_ctor_vm_procedure3_spec >(builtin_default_ctor_vm_procedure3_impl);
    m_graph.register_handler_function< builtin_dtor_vm_procedure3_spec >(builtin_dtor_vm_procedure3_impl);
    m_graph.register_handler_function< builtin_move_ctor_vm_procedure3_spec >(builtin_move_ctor_vm_procedure3_impl);
    m_graph.register_handler_function< builtin_swap_vm_procedure3_spec >(builtin_swap_vm_procedure3_impl);
    m_graph.register_handler_function< builtin_template_instanciation_spec >(builtin_template_instanciation_impl);
    m_graph.register_handler_function< builtin_vm_procedure3_spec >(builtin_vm_procedure3_impl);
    m_graph.register_handler_function< class_builtin_spec >(class_builtin_impl);
    m_graph.register_handler_function< class_default_ctor_spec >(class_default_ctor_impl);
    m_graph.register_handler_function< class_default_dtor_spec >(class_default_dtor_impl);
    m_graph.register_handler_function< class_field_declaration_list_spec >(class_field_declaration_list_impl);
    m_graph.register_handler_function< class_field_list_spec >(class_field_list_impl);
    m_graph.register_handler_function< class_layout_spec >(class_layout_impl);
    m_graph.register_handler_function< class_requires_gen_assignment_spec >(class_requires_gen_assignment_impl);
    m_graph.register_handler_function< class_requires_gen_copy_ctor_spec >(class_requires_gen_copy_ctor_impl);
    m_graph.register_handler_function< class_requires_gen_default_ctor_spec >(class_requires_gen_default_ctor_impl);
    m_graph.register_handler_function< class_requires_gen_default_dtor_spec >(class_requires_gen_default_dtor_impl);
    m_graph.register_handler_function< class_requires_gen_move_ctor_spec >(class_requires_gen_move_ctor_impl);
    m_graph.register_handler_function< class_requires_gen_swap_spec >(class_requires_gen_swap_impl);
    m_graph.register_handler_function< class_tags_spec >(class_tags_impl);
    m_graph.register_handler_function< class_trivially_constructible_spec >(class_trivially_constructible_impl);
    m_graph.register_handler_function< class_trivially_destructible_spec >(class_trivially_destructible_impl);
    m_graph.register_handler_function< constexpr_bool_spec >(constexpr_bool_impl);
    m_graph.register_handler_function< constexpr_eval_antestatal_spec >(constexpr_eval_antestatal_impl);
    m_graph.register_handler_function< constexpr_eval_string_spec >(constexpr_eval_string_impl);
    m_graph.register_handler_function< constexpr_eval_v3_spec >(constexpr_eval_v3_impl);
    m_graph.register_handler_function< constexpr_eval_spec >(constexpr_eval_impl);
    m_graph.register_handler_function< constexpr_routine_antestatal_spec >(constexpr_routine_antestatal_impl);
    m_graph.register_handler_function< constexpr_routine_v3_spec >(constexpr_routine_v3_impl);
    m_graph.register_handler_function< constexpr_routine_spec >(constexpr_routine_impl);
    m_graph.register_handler_function< constexpr_u64_spec >(constexpr_u64_impl);
    m_graph.register_handler_function< convertible_by_call_spec >(convertible_by_call_impl);
    m_graph.register_handler_function< declaroids_spec >(declaroids_impl);
    m_graph.register_handler_function< ensig_argument_initialize_spec >(ensig_argument_initialize_impl);
    m_graph.register_handler_function< ensig_initialize_spec >(ensig_initialize_impl);
    m_graph.register_handler_function< ensig_tempars_spec >(ensig_tempars_impl);
    m_graph.register_handler_function< exists_spec >(exists_impl);
    m_graph.register_handler_function< extern_linksymbol_spec >(extern_linksymbol_impl);
    m_graph.register_handler_function< functanoid_return_type_spec >(functanoid_return_type_impl);
    m_graph.register_handler_function< functanoid_sigtype_spec >(functanoid_sigtype_impl);
    m_graph.register_handler_function< function_builtin_spec >(function_builtin_impl);
    m_graph.register_handler_function< function_declaration_spec >(function_declaration_impl);
    m_graph.register_handler_function< function_ensig_init_with_spec >(function_ensig_init_with_impl);
    m_graph.register_handler_function< function_instanciation_spec >(function_instanciation_impl);
    m_graph.register_handler_function< function_pack_info_spec >(function_pack_info_impl);
    m_graph.register_handler_function< function_param_names_spec >(function_param_names_impl);
    m_graph.register_handler_function< function_positional_parameter_names_spec >(function_positional_parameter_names_impl);
    m_graph.register_handler_function< function_primitive_spec >(function_primitive_impl);
    m_graph.register_handler_function< functum_builtin_overloads_spec >(functum_builtin_overloads_impl);
    m_graph.register_handler_function< functum_builtins_spec >(functum_builtins_impl);
    m_graph.register_handler_function< functum_exists_and_is_callable_with_spec >(functum_exists_and_is_callable_with_impl);
    m_graph.register_handler_function< functum_initialize_spec >(functum_initialize_impl);
    m_graph.register_handler_function< functum_list_user_ensig_declarations_spec >(functum_list_user_ensig_declarations_impl);
    m_graph.register_handler_function< functum_list_user_overload_declarations_spec >(functum_list_user_overload_declarations_impl);
    m_graph.register_handler_function< functum_map_user_formal_ensigs_spec >(functum_map_user_formal_ensigs_impl);
    m_graph.register_handler_function< functum_overloads_spec >(functum_overloads_impl);
    m_graph.register_handler_function< functum_select_function_spec >(functum_select_function_impl);
    m_graph.register_handler_function< functum_user_overloads_spec >(functum_user_overloads_impl);
    m_graph.register_handler_function< global_is_antestatal_static_spec >(global_is_antestatal_static_impl);
    m_graph.register_handler_function< global_is_serialoid_static_spec >(global_is_serialoid_static_impl);
    m_graph.register_handler_function< global_is_string_static_spec >(global_is_string_static_impl);
    m_graph.register_handler_function< have_nontrivial_member_ctor_spec >(have_nontrivial_member_ctor_impl);
    m_graph.register_handler_function< have_nontrivial_member_dtor_spec >(have_nontrivial_member_dtor_impl);
    m_graph.register_handler_function< implicitly_convertible_to_spec >(implicitly_convertible_to_impl);
    m_graph.register_handler_function< instanciation_spec >(instanciation_impl);
    m_graph.register_handler_function< instanciation_tempar_map_spec >(instanciation_tempar_map_impl);
    m_graph.register_handler_function< interpret_bool_spec >(interpret_bool_impl);
    m_graph.register_handler_function< interpret_value_spec >(interpret_value_impl);
    m_graph.register_handler_function< list_builtin_constructors_spec >(list_builtin_constructors_impl);
    m_graph.register_handler_function< list_static_tests_spec >(list_static_tests_impl);
    m_graph.register_handler_function< list_user_functum_formal_paratypes_spec >(list_user_functum_formal_paratypes_impl);
    m_graph.register_handler_function< lookup_spec >(lookup_impl);
    m_graph.register_handler_function< module_ast_spec >(module_ast_impl);
    m_graph.register_handler_function< module_options_map_spec >(module_options_map_impl);
    m_graph.register_handler_function< module_source_name_spec >(module_source_name_impl);
    m_graph.register_handler_function< module_sources_spec >(module_sources_impl);
    m_graph.register_handler_function< parse_file_spec >(parse_file_impl);
    m_graph.register_handler_function< procedure_linksymbol_spec >(procedure_linksymbol_impl);
    m_graph.register_handler_function< run_static_test_spec >(run_static_test_impl);
    m_graph.register_handler_function< run_static_tests_spec >(run_static_tests_impl);
    m_graph.register_handler_function< serialoid_static_value_spec >(serialoid_static_value_impl);
    m_graph.register_handler_function< sintpointer_type_spec >(sintpointer_type_impl);
    m_graph.register_handler_function< source_file_id_spec >(source_file_id_impl);
    m_graph.register_handler_function< source_file_index_spec >(source_file_index_impl);
    m_graph.register_handler_function< source_file_name_spec >(source_file_name_impl);
    m_graph.register_handler_function< string_static_value_spec >(string_static_value_impl);
    m_graph.register_handler_function< symboid_spec >(symboid_impl);
    m_graph.register_handler_function< symboid_subdeclaroids_spec >(symboid_subdeclaroids_impl);
    m_graph.register_handler_function< symbol_tempars_spec >(symbol_tempars_impl);
    m_graph.register_handler_function< symbol_type_spec >(symbol_type_impl);
    m_graph.register_handler_function< template_builtin_spec >(template_builtin_impl);
    m_graph.register_handler_function< template_instanciation_spec >(template_instanciation_impl);
    m_graph.register_handler_function< templex_builtins_spec >(templex_builtins_impl);
    m_graph.register_handler_function< templex_builtin_templates_spec >(templex_builtin_templates_impl);
    m_graph.register_handler_function< templex_initialize_spec >(templex_initialize_impl);
    m_graph.register_handler_function< templex_select_template_spec >(templex_select_template_impl);
    m_graph.register_handler_function< type_is_antestatal_spec >(type_is_antestatal_impl);
    m_graph.register_handler_function< type_is_serialoid_spec >(type_is_serialoid_impl);
    m_graph.register_handler_function< type_is_stringlike_spec >(type_is_stringlike_impl);
    m_graph.register_handler_function< type_is_implicitly_datatype_spec >(type_is_implicitly_datatype_impl);
    m_graph.register_handler_function< type_placement_info_spec >(type_placement_info_impl);
    m_graph.register_handler_function< type_should_autogen_deserialize_spec >(type_should_autogen_deserialize_impl);
    m_graph.register_handler_function< type_should_autogen_serialize_spec >(type_should_autogen_serialize_impl);
    m_graph.register_handler_function< uintpointer_type_spec >(uintpointer_type_impl);
    m_graph.register_handler_function< user_assignment_exists_spec >(user_assignment_exists_impl);
    m_graph.register_handler_function< user_copy_ctor_exists_spec >(user_copy_ctor_exists_impl);
    m_graph.register_handler_function< user_default_ctor_exists_spec >(user_default_ctor_exists_impl);
    m_graph.register_handler_function< user_default_dtor_exists_spec >(user_default_dtor_exists_impl);
    m_graph.register_handler_function< user_deserialize_exists_spec >(user_deserialize_exists_impl);
    m_graph.register_handler_function< user_move_ctor_exists_spec >(user_move_ctor_exists_impl);
    m_graph.register_handler_function< user_serialize_exists_spec >(user_serialize_exists_impl);
    m_graph.register_handler_function< user_swap_exists_spec >(user_swap_exists_impl);
    m_graph.register_handler_function< user_vm_procedure3_spec >(user_vm_procedure3_impl);
    m_graph.register_handler_function< variable_type_spec >(variable_type_impl);
    m_graph.register_handler_function< vm_procedure3_spec >(vm_procedure3_impl);

    m_graph.bind_handlers();
}

quxlang::compiler_querygraph::~compiler_querygraph() = default;

void quxlang::compiler_querygraph::write_dump_file()
{
    if (!m_dump_output_path.has_value())
    {
        return;
    }

    write_marshaled_graph_dump(m_graph, *m_dump_output_path);
    if (!m_has_reported_dump_output_path)
    {
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::cout << "Quxlang graph dump written to: " << m_dump_output_path->string() << std::endl;
        }
        m_has_reported_dump_output_path = true;
    }
}
