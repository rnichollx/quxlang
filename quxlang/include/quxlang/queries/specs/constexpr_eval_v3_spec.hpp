// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_V3_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_V3_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/struct_layout.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/constexpr_routine_v3.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/functanoid_directly_instantiated_functanoids.hpp>
#include <quxlang/queries/functanoid_required_struct_layouts.hpp>
#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct constexpr_eval_v3_spec
    {
        using query = constexpr_eval_v3_query;
        using dependencies = rpnx::typelist< antestatal_static_value_query, struct_layout_query, constexpr_routine_v3_query, enum_info_query, flagset_info_query, functanoid_directly_instantiated_functanoids_query, functanoid_required_struct_layouts_query, global_init_type_query, global_is_antestatal_static_query, source_bundle_query, source_file_index_query, symboid_query, class_type_query, symbol_type_query, variable_type_query, vm_procedure3_query >;
    };

    /// Evaluates a constexpr v3 expression and returns all requested result IDs.
    rpnx::querygraph::coroutine< constexpr_eval_v3_spec > constexpr_eval_v3_impl(constexpr_input_v3 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_V3_SPEC_HEADER_GUARD
