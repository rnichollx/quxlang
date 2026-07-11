// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <querygraph_ui.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace
{
    template < typename T >
    void register_type(rpnx::querygraph::ui::graph_dump_node_viewer& viewer)
    {
        viewer.copy_binary_spec_and_text_descriptor(
            rpnx::serial4::get_noncryptographic_typehash< T >(),
            [](std::vector< std::byte > const& data) -> std::any
            {
                T value{};
                auto it = data.begin();
                rpnx::serial4::deserialize_iter(value, it, data.end());
                return std::any(std::move(value));
            },
            [](std::any value, std::any) -> std::string
            {
                return rpnx::querygraph::debug_traits< T >::to_debug_string(std::any_cast< T >(value));
            });
    }

    void register_quxlang_io_types(rpnx::querygraph::ui::graph_dump_node_viewer& viewer)
    {
        register_type< quxlang::source_bundle_query::input_type >(viewer);
        register_type< quxlang::source_bundle_query::output_type >(viewer);
        register_type< quxlang::machine_info_query::input_type >(viewer);
        register_type< quxlang::machine_info_query::output_type >(viewer);
        register_type< quxlang::module_source_name_map_query::input_type >(viewer);
        register_type< quxlang::module_source_name_map_query::output_type >(viewer);
        register_type< quxlang::argument_adaptation_is_better_fit_spec::query_spec::input_type >(viewer);
        register_type< quxlang::argument_adaptation_is_better_fit_spec::query_spec::output_type >(viewer);
        register_type< quxlang::argument_adaptation_rank_spec::query_spec::input_type >(viewer);
        register_type< quxlang::argument_adaptation_rank_spec::query_spec::output_type >(viewer);
        register_type< quxlang::argument_initialize_by_class_conversion_spec::query_spec::input_type >(viewer);
        register_type< quxlang::argument_initialize_by_class_conversion_spec::query_spec::output_type >(viewer);
        register_type< quxlang::argument_initialize_by_intrinsic_spec::query_spec::input_type >(viewer);
        register_type< quxlang::argument_initialize_by_intrinsic_spec::query_spec::output_type >(viewer);
        register_type< quxlang::argument_initialize_by_template_spec::query_spec::input_type >(viewer);
        register_type< quxlang::argument_initialize_by_template_spec::query_spec::output_type >(viewer);
        register_type< quxlang::asm_procedure_from_symbol_spec::query_spec::input_type >(viewer);
        register_type< quxlang::asm_procedure_from_symbol_spec::query_spec::output_type >(viewer);
        register_type< quxlang::bindable_spec::query_spec::input_type >(viewer);
        register_type< quxlang::bindable_spec::query_spec::output_type >(viewer);
        register_type< quxlang::bindable_by_reference_objectization_spec::query_spec::input_type >(viewer);
        register_type< quxlang::bindable_by_reference_objectization_spec::query_spec::output_type >(viewer);
        register_type< quxlang::bindable_by_reference_requalification_spec::query_spec::input_type >(viewer);
        register_type< quxlang::bindable_by_reference_requalification_spec::query_spec::output_type >(viewer);
        register_type< quxlang::bindable_by_temporary_materialization_spec::query_spec::input_type >(viewer);
        register_type< quxlang::bindable_by_temporary_materialization_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_assignment_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_assignment_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_copy_ctor_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_copy_ctor_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_datatype_compare_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_datatype_compare_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_default_ctor_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_default_ctor_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_dtor_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_dtor_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_move_ctor_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_move_ctor_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_swap_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_swap_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::builtin_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::builtin_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_builtin_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_builtin_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_default_ctor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_default_ctor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_type_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_type_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_default_dtor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_default_dtor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::struct_field_declaration_list_spec::query_spec::input_type >(viewer);
        register_type< quxlang::struct_field_declaration_list_spec::query_spec::output_type >(viewer);
        register_type< quxlang::struct_field_list_spec::query_spec::input_type >(viewer);
        register_type< quxlang::struct_field_list_spec::query_spec::output_type >(viewer);
        register_type< quxlang::struct_layout_spec::query_spec::input_type >(viewer);
        register_type< quxlang::struct_layout_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_requires_gen_assignment_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_requires_gen_assignment_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_requires_gen_copy_ctor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_requires_gen_copy_ctor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_requires_gen_default_ctor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_requires_gen_default_ctor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_requires_gen_default_dtor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_requires_gen_default_dtor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_requires_gen_move_ctor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_requires_gen_move_ctor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_requires_gen_swap_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_requires_gen_swap_spec::query_spec::output_type >(viewer);
        register_type< quxlang::struct_tags_spec::query_spec::input_type >(viewer);
        register_type< quxlang::struct_tags_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_trivially_constructible_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_trivially_constructible_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_trivially_destructible_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_trivially_destructible_spec::query_spec::output_type >(viewer);
        register_type< quxlang::constexpr_bool_spec::query_spec::input_type >(viewer);
        register_type< quxlang::constexpr_bool_spec::query_spec::output_type >(viewer);
        register_type< quxlang::constexpr_eval_spec::query_spec::input_type >(viewer);
        register_type< quxlang::constexpr_eval_spec::query_spec::output_type >(viewer);
        register_type< quxlang::constexpr_routine_spec::query_spec::input_type >(viewer);
        register_type< quxlang::constexpr_routine_spec::query_spec::output_type >(viewer);
        register_type< quxlang::constexpr_u64_spec::query_spec::input_type >(viewer);
        register_type< quxlang::constexpr_u64_spec::query_spec::output_type >(viewer);
        register_type< quxlang::convertible_by_call_spec::query_spec::input_type >(viewer);
        register_type< quxlang::convertible_by_call_spec::query_spec::output_type >(viewer);
        register_type< quxlang::declaroids_spec::query_spec::input_type >(viewer);
        register_type< quxlang::declaroids_spec::query_spec::output_type >(viewer);
        register_type< quxlang::ensig_argument_initialize_spec::query_spec::input_type >(viewer);
        register_type< quxlang::ensig_argument_initialize_spec::query_spec::output_type >(viewer);
        register_type< quxlang::ensig_tempars_spec::query_spec::input_type >(viewer);
        register_type< quxlang::ensig_tempars_spec::query_spec::output_type >(viewer);
        register_type< quxlang::exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::extern_linksymbol_spec::query_spec::input_type >(viewer);
        register_type< quxlang::extern_linksymbol_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functanoid_return_type_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functanoid_return_type_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functanoid_sigtype_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functanoid_sigtype_spec::query_spec::output_type >(viewer);
        register_type< quxlang::function_builtin_spec::query_spec::input_type >(viewer);
        register_type< quxlang::function_builtin_spec::query_spec::output_type >(viewer);
        register_type< quxlang::function_declaration_spec::query_spec::input_type >(viewer);
        register_type< quxlang::function_declaration_spec::query_spec::output_type >(viewer);
        register_type< quxlang::function_ensig_init_with_spec::query_spec::input_type >(viewer);
        register_type< quxlang::function_ensig_init_with_spec::query_spec::output_type >(viewer);
        register_type< quxlang::function_instanciation_spec::query_spec::input_type >(viewer);
        register_type< quxlang::function_instanciation_spec::query_spec::output_type >(viewer);
        register_type< quxlang::function_param_names_spec::query_spec::input_type >(viewer);
        register_type< quxlang::function_param_names_spec::query_spec::output_type >(viewer);
        register_type< quxlang::function_positional_parameter_names_spec::query_spec::input_type >(viewer);
        register_type< quxlang::function_positional_parameter_names_spec::query_spec::output_type >(viewer);
        register_type< quxlang::function_primitive_spec::query_spec::input_type >(viewer);
        register_type< quxlang::function_primitive_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_builtin_overloads_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_builtin_overloads_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_builtins_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_builtins_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_exists_and_is_callable_with_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_exists_and_is_callable_with_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_initialize_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_initialize_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_list_user_ensig_declarations_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_list_user_ensig_declarations_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_list_user_overload_declarations_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_list_user_overload_declarations_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_map_user_formal_ensigs_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_map_user_formal_ensigs_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_overloads_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_overloads_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_select_function_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_select_function_spec::query_spec::output_type >(viewer);
        register_type< quxlang::functum_user_overloads_spec::query_spec::input_type >(viewer);
        register_type< quxlang::functum_user_overloads_spec::query_spec::output_type >(viewer);
        register_type< quxlang::have_nontrivial_member_ctor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::have_nontrivial_member_ctor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::have_nontrivial_member_dtor_spec::query_spec::input_type >(viewer);
        register_type< quxlang::have_nontrivial_member_dtor_spec::query_spec::output_type >(viewer);
        register_type< quxlang::implicitly_convertible_to_spec::query_spec::input_type >(viewer);
        register_type< quxlang::implicitly_convertible_to_spec::query_spec::output_type >(viewer);
        register_type< quxlang::instanciation_spec::query_spec::input_type >(viewer);
        register_type< quxlang::instanciation_spec::query_spec::output_type >(viewer);
        register_type< quxlang::instanciation_tempar_map_spec::query_spec::input_type >(viewer);
        register_type< quxlang::instanciation_tempar_map_spec::query_spec::output_type >(viewer);
        register_type< quxlang::interpret_bool_spec::query_spec::input_type >(viewer);
        register_type< quxlang::interpret_bool_spec::query_spec::output_type >(viewer);
        register_type< quxlang::interpret_value_spec::query_spec::input_type >(viewer);
        register_type< quxlang::interpret_value_spec::query_spec::output_type >(viewer);
        register_type< quxlang::list_builtin_constructors_spec::query_spec::input_type >(viewer);
        register_type< quxlang::list_builtin_constructors_spec::query_spec::output_type >(viewer);
        register_type< quxlang::list_static_tests_spec::query_spec::input_type >(viewer);
        register_type< quxlang::list_static_tests_spec::query_spec::output_type >(viewer);
        register_type< quxlang::list_user_functum_formal_paratypes_spec::query_spec::input_type >(viewer);
        register_type< quxlang::list_user_functum_formal_paratypes_spec::query_spec::output_type >(viewer);
        register_type< quxlang::lookup_spec::query_spec::input_type >(viewer);
        register_type< quxlang::lookup_spec::query_spec::output_type >(viewer);
        register_type< quxlang::module_ast_spec::query_spec::input_type >(viewer);
        register_type< quxlang::module_ast_spec::query_spec::output_type >(viewer);
        register_type< quxlang::module_source_name_spec::query_spec::input_type >(viewer);
        register_type< quxlang::module_source_name_spec::query_spec::output_type >(viewer);
        register_type< quxlang::module_sources_spec::query_spec::input_type >(viewer);
        register_type< quxlang::module_sources_spec::query_spec::output_type >(viewer);
        register_type< quxlang::procedure_linksymbol_spec::query_spec::input_type >(viewer);
        register_type< quxlang::procedure_linksymbol_spec::query_spec::output_type >(viewer);
        register_type< quxlang::sintpointer_type_spec::query_spec::input_type >(viewer);
        register_type< quxlang::sintpointer_type_spec::query_spec::output_type >(viewer);
        register_type< quxlang::symboid_spec::query_spec::input_type >(viewer);
        register_type< quxlang::symboid_spec::query_spec::output_type >(viewer);
        register_type< quxlang::symboid_subdeclaroids_spec::query_spec::input_type >(viewer);
        register_type< quxlang::symboid_subdeclaroids_spec::query_spec::output_type >(viewer);
        register_type< quxlang::symbol_tempars_spec::query_spec::input_type >(viewer);
        register_type< quxlang::symbol_tempars_spec::query_spec::output_type >(viewer);
        register_type< quxlang::symbol_type_spec::query_spec::input_type >(viewer);
        register_type< quxlang::symbol_type_spec::query_spec::output_type >(viewer);
        register_type< quxlang::template_instanciation_spec::query_spec::input_type >(viewer);
        register_type< quxlang::template_instanciation_spec::query_spec::output_type >(viewer);
        register_type< quxlang::temploid_formal_ensig_spec::query_spec::input_type >(viewer);
        register_type< quxlang::temploid_formal_ensig_spec::query_spec::output_type >(viewer);
        register_type< quxlang::templex_initialize_spec::query_spec::input_type >(viewer);
        register_type< quxlang::templex_initialize_spec::query_spec::output_type >(viewer);
        register_type< quxlang::type_is_implicitly_datatype_spec::query_spec::input_type >(viewer);
        register_type< quxlang::type_is_implicitly_datatype_spec::query_spec::output_type >(viewer);
        register_type< quxlang::class_placement_info_spec::query_spec::input_type >(viewer);
        register_type< quxlang::class_placement_info_spec::query_spec::output_type >(viewer);
        register_type< quxlang::type_should_autogen_deserialize_spec::query_spec::input_type >(viewer);
        register_type< quxlang::type_should_autogen_deserialize_spec::query_spec::output_type >(viewer);
        register_type< quxlang::type_should_autogen_serialize_spec::query_spec::input_type >(viewer);
        register_type< quxlang::type_should_autogen_serialize_spec::query_spec::output_type >(viewer);
        register_type< quxlang::uintpointer_type_spec::query_spec::input_type >(viewer);
        register_type< quxlang::uintpointer_type_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_assignment_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_assignment_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_copy_ctor_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_copy_ctor_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_default_ctor_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_default_ctor_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_default_dtor_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_default_dtor_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_deserialize_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_deserialize_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_move_ctor_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_move_ctor_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_serialize_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_serialize_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_swap_exists_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_swap_exists_spec::query_spec::output_type >(viewer);
        register_type< quxlang::user_vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::user_vm_procedure3_spec::query_spec::output_type >(viewer);
        register_type< quxlang::variable_type_spec::query_spec::input_type >(viewer);
        register_type< quxlang::variable_type_spec::query_spec::output_type >(viewer);
        register_type< quxlang::vm_procedure3_spec::query_spec::input_type >(viewer);
        register_type< quxlang::vm_procedure3_spec::query_spec::output_type >(viewer);
    }

    auto read_dump_file(std::filesystem::path const& dump_path) -> std::vector< std::byte >
    {
        std::ifstream in(dump_path, std::ios::binary);
        if (!in)
        {
            throw quxlang::compilation_error(std::format("Failed to open dump file: {}", dump_path.string()));
        }

        auto const file_size = std::filesystem::file_size(dump_path);
        std::vector< std::byte > dump_bytes(file_size);
        if (!dump_bytes.empty())
        {
            in.read(reinterpret_cast< char* >(dump_bytes.data()), static_cast< std::streamsize >(dump_bytes.size()));
            if (!in)
            {
                throw quxlang::compilation_error(std::format("Failed to read dump file: {}", dump_path.string()));
            }
        }

        return dump_bytes;
    }

} // namespace

int main(int argc, char** argv)
{
    if (argc > 2)
    {
        std::cerr << "Usage: quxlang-debugger-ui [dump-file.qgd]" << std::endl;
        return 1;
    }

    try
    {
        rpnx::querygraph::ui::graph_dump_node_viewer viewer;
        register_quxlang_io_types(viewer);

        if (argc == 2)
        {
            std::filesystem::path const dump_path = argv[1];
            viewer.set_marshaled_dump_data(read_dump_file(dump_path));
        }

        viewer.run();
    }
    catch (std::exception const& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
