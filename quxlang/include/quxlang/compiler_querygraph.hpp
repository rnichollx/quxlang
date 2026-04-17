// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_COMPILER_QUERYGRAPH_HEADER_GUARD
#define QUXLANG_COMPILER_QUERYGRAPH_HEADER_GUARD

#include <quxlang/data/machine.hpp>
#include <quxlang/data/target_configuration.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_option_strings_map.hpp>
#include <quxlang/queries/module_options_map.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/querygraph_traits.hpp>
#include <quxlang/queries/source_bundle.hpp>
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
#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_spec.hpp>
#include <quxlang/queries/specs/constexpr_u64_spec.hpp>
#include <quxlang/queries/specs/convertible_by_call_spec.hpp>
#include <quxlang/queries/specs/declaroids_spec.hpp>
#include <quxlang/queries/specs/ensig_argument_initialize_spec.hpp>
#include <quxlang/queries/specs/ensig_tempars_spec.hpp>
#include <quxlang/queries/specs/exists_spec.hpp>
#include <quxlang/queries/specs/extern_linksymbol_spec.hpp>
#include <quxlang/queries/specs/functanoid_return_type_spec.hpp>
#include <quxlang/queries/specs/functanoid_sigtype_spec.hpp>
#include <quxlang/queries/specs/function_builtin_spec.hpp>
#include <quxlang/queries/specs/function_declaration_spec.hpp>
#include <quxlang/queries/specs/function_ensig_init_with_spec.hpp>
#include <quxlang/queries/specs/function_instanciation_spec.hpp>
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
#include <quxlang/queries/specs/procedure_linksymbol_spec.hpp>
#include <quxlang/queries/specs/run_static_test_spec.hpp>
#include <quxlang/queries/specs/run_static_tests_spec.hpp>
#include <quxlang/queries/specs/sintpointer_type_spec.hpp>
#include <quxlang/queries/specs/source_file_id_spec.hpp>
#include <quxlang/queries/specs/source_file_index_spec.hpp>
#include <quxlang/queries/specs/source_file_name_spec.hpp>
#include <quxlang/queries/specs/symboid_spec.hpp>
#include <quxlang/queries/specs/symboid_subdeclaroids_spec.hpp>
#include <quxlang/queries/specs/symbol_tempars_spec.hpp>
#include <quxlang/queries/specs/symbol_type_spec.hpp>
#include <quxlang/queries/specs/template_instanciation_spec.hpp>
#include <quxlang/queries/specs/templex_initialize_spec.hpp>
#include <quxlang/queries/specs/type_is_antestatal_spec.hpp>
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

#include <rpnx/querygraph/querygraph.hpp>

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <utility>

namespace quxlang
{
    class compiler_querygraph
    {
      public:
        compiler_querygraph(source_bundle const& bundle, std::string configured_target, output_info const& machine_info,
                            std::optional< std::filesystem::path > dump_output_path = std::nullopt);
        ~compiler_querygraph();

        template < typename Query >
        auto make_request(typename Query::input_type input) -> typename Query::output_type
        {
            auto result = m_graph.make_request< Query >(std::move(input));
            write_dump_file();
            return result;
        }

        auto raw_graph() -> decltype(auto)
        {
            return (m_graph);
        }

      private:
        void write_dump_file();

        rpnx::querygraph::graph m_graph;
        std::optional< std::filesystem::path > m_dump_output_path;
        bool m_has_reported_dump_output_path = false;
    };
} // namespace quxlang

#endif // QUXLANG_COMPILER_QUERYGRAPH_HEADER_GUARD
