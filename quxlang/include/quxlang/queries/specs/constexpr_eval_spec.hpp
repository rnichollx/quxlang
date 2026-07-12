// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/struct_layout.hpp>
#include <quxlang/queries/constexpr_eval.hpp>
#include <quxlang/queries/constexpr_routine.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/fusion_layout.hpp>
#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <quxlang/queries/vm_procedure3.hpp>
#include <quxlang/queries/vmir_dependencies.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct constexpr_eval_spec
    {
        using query = constexpr_eval_query;
        using dependencies = rpnx::typelist< antestatal_static_value_query, struct_layout_query, constexpr_routine_query, enum_info_query, flagset_info_query, fusion_layout_query, global_init_type_query, global_is_antestatal_static_query, source_bundle_query, source_file_index_query, symboid_query, class_type_query, union_info_query, variable_type_query, variant_info_query, vm_procedure3_query, direct_dependencies_query >;
    };

    rpnx::querygraph::coroutine< constexpr_eval_spec > constexpr_eval_impl(constexpr_input2 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_SPEC_HEADER_GUARD
