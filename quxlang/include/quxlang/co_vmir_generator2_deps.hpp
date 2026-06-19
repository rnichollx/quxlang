// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CO_VMIR_GENERATOR2_DEPS_HEADER_GUARD
#define QUXLANG_CO_VMIR_GENERATOR2_DEPS_HEADER_GUARD

#include <quxlang/queries/builtin_assignment_vm_procedure3.hpp>
#include <quxlang/queries/builtin_copy_ctor_vm_procedure3.hpp>
#include <quxlang/queries/builtin_datatype_compare_vm_procedure3.hpp>
#include <quxlang/queries/builtin_default_ctor_vm_procedure3.hpp>
#include <quxlang/queries/builtin_dtor_vm_procedure3.hpp>
#include <quxlang/queries/builtin_move_ctor_vm_procedure3.hpp>
#include <quxlang/queries/builtin_swap_vm_procedure3.hpp>
#include <quxlang/queries/class_default_dtor.hpp>
#include <quxlang/queries/class_field_list.hpp>
#include <quxlang/queries/class_layout.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/ensig_argument_initialize.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/functanoid_return_type.hpp>
#include <quxlang/queries/functanoid_sigtype.hpp>
#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/function_ensig_init_with.hpp>
#include <quxlang/queries/function_pack_info.hpp>
#include <quxlang/queries/function_param_names.hpp>
#include <quxlang/queries/function_primitive.hpp>
#include <quxlang/queries/functum_overloads.hpp>
#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/global_is_per_thread.hpp>
#include <quxlang/queries/global_is_serialoid_static.hpp>
#include <quxlang/queries/global_is_string_static.hpp>
#include <quxlang/queries/implementation_function_map.hpp>
#include <quxlang/queries/implementation_interface_type.hpp>
#include <quxlang/queries/implicitly_convertible_to.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/instanciation_concrete_params.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_options_map.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/serialoid_static_value.hpp>
#include <quxlang/queries/string_static_value.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>
#include <quxlang/queries/template_builtin.hpp>
#include <quxlang/queries/type_placement_info.hpp>
#include <quxlang/queries/type_is_implicitly_datatype.hpp>
#include <quxlang/queries/type_is_serialoid.hpp>
#include <quxlang/queries/type_is_stringlike.hpp>
#include <quxlang/queries/uintpointer_type.hpp>
#include <quxlang/queries/variable_type.hpp>

#include <rpnx/typelist.hpp>

namespace quxlang
{
    using co_vmir_generator2_query_deps = rpnx::typelist<
        class_default_dtor_query,
        class_field_list_query,
        class_layout_query,
        constexpr_bool_query,
        constexpr_eval_v3_query,
        constexpr_u64_query,
        ensig_argument_initialize_query,
        enum_info_query,
        flagset_info_query,
        functanoid_return_type_query,
        functanoid_sigtype_query,
        function_builtin_query,
        function_declaration_query,
        function_ensig_init_with_query,
        function_pack_info_query,
        function_param_names_query,
        function_primitive_query,
        functum_overloads_query,
        global_init_type_query,
        global_is_antestatal_static_query,
        global_is_per_thread_query,
        global_is_serialoid_static_query,
        global_is_string_static_query,
        implementation_function_map_query,
        implementation_interface_type_query,
        implicitly_convertible_to_qg_query,
        instanciation_query,
        instanciation_concrete_params_query,
        interface_slot_list_query,
        lookup_query,
        machine_info_query,
        module_options_map_query,
        symboid_query,
        serialoid_static_value_query,
        string_static_value_query,
        symbol_type_query,
        temploid_formal_ensig_query,
        template_builtin_query,
        type_placement_info_query,
        type_is_serialoid_query,
        type_is_stringlike_query,
        uintpointer_type_query,
        variable_type_query >;

    using co_vmir_generator2_builtin_vm_query_deps = rpnx::typelist<
        class_default_dtor_query,
        class_field_list_query,
        class_layout_query,
        constexpr_bool_query,
        constexpr_eval_v3_query,
        constexpr_u64_query,
        ensig_argument_initialize_query,
        enum_info_query,
        flagset_info_query,
        functanoid_return_type_query,
        functanoid_sigtype_query,
        function_builtin_query,
        function_declaration_query,
        function_ensig_init_with_query,
        function_pack_info_query,
        function_param_names_query,
        function_primitive_query,
        functum_overloads_query,
        global_init_type_query,
        global_is_antestatal_static_query,
        global_is_per_thread_query,
        global_is_serialoid_static_query,
        global_is_string_static_query,
        implementation_function_map_query,
        implementation_interface_type_query,
        implicitly_convertible_to_qg_query,
        instanciation_query,
        instanciation_concrete_params_query,
        interface_slot_list_query,
        lookup_query,
        machine_info_query,
        module_options_map_query,
        symboid_query,
        serialoid_static_value_query,
        string_static_value_query,
        symbol_type_query,
        temploid_formal_ensig_query,
        template_builtin_query,
        type_placement_info_query,
        type_is_serialoid_query,
        type_is_stringlike_query,
        uintpointer_type_query,
        variable_type_query,
        builtin_assignment_vm_procedure3_query,
        builtin_copy_ctor_vm_procedure3_query,
        builtin_datatype_compare_vm_procedure3_query,
        builtin_default_ctor_vm_procedure3_query,
        builtin_dtor_vm_procedure3_query,
        builtin_move_ctor_vm_procedure3_query,
        builtin_swap_vm_procedure3_query,
        type_is_implicitly_datatype_query >;
} // namespace quxlang

#endif // QUXLANG_CO_VMIR_GENERATOR2_DEPS_HEADER_GUARD
