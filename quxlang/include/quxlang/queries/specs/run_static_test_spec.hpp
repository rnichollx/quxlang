// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_RUN_STATIC_TEST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_RUN_STATIC_TEST_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/class_default_dtor.hpp>
#include <quxlang/queries/class_field_list.hpp>
#include <quxlang/queries/class_layout.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/functanoid_return_type.hpp>
#include <quxlang/queries/functanoid_sigtype.hpp>
#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/function_pack_info.hpp>
#include <quxlang/queries/function_param_names.hpp>
#include <quxlang/queries/functum_overloads.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/implicitly_convertible_to.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_options_map.hpp>
#include <quxlang/queries/run_static_test.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_placement_info.hpp>
#include <quxlang/queries/uintpointer_type.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using run_static_test_spec = rpnx::querygraph::query_handler_spec<
        run_static_test_query,
        rpnx::typelist<
            antestatal_static_value_query,
            class_default_dtor_query,
            class_field_list_query,
            class_layout_query,
            constexpr_bool_query,
            constexpr_eval_v3_query,
            constexpr_u64_query,
            functanoid_return_type_query,
            functanoid_sigtype_query,
            function_builtin_query,
            function_declaration_query,
            function_pack_info_query,
            function_param_names_query,
            functum_overloads_query,
            global_is_antestatal_static_query,
            implicitly_convertible_to_qg_query,
            instanciation_query,
            lookup_query,
            machine_info_query,
            module_options_map_query,
            source_bundle_query,
            source_file_index_query,
            symboid_query,
            symbol_type_query,
            type_placement_info_query,
            uintpointer_type_query,
            variable_type_query,
            vm_procedure3_query > >;

    rpnx::querygraph::coroutine< run_static_test_spec > run_static_test_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_RUN_STATIC_TEST_SPEC_HEADER_GUARD
