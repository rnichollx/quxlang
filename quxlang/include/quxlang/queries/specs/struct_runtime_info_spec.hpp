// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_RUNTIME_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_RUNTIME_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/functum_initialize.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/struct_inheritance_info.hpp>
#include <quxlang/queries/struct_layout.hpp>
#include <quxlang/queries/struct_runtime_info.hpp>
#include <quxlang/queries/struct_runtime_requirements.hpp>
#include <quxlang/queries/struct_virtual_slots.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_runtime_info_spec
    {
        using query = struct_runtime_info_query;
        using dependencies = rpnx::typelist< class_type_query, functum_initialize_query, struct_field_list_query, struct_inheritance_info_query, struct_layout_query, struct_runtime_requirements_query, struct_virtual_slots_query, symbol_type_query >;
    };

    /** Returns backend-ready runtime descriptors for one complete struct type. */
    auto struct_runtime_info_impl(type_symbol input) -> rpnx::querygraph::coroutine< struct_runtime_info_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_RUNTIME_INFO_SPEC_HEADER_GUARD
