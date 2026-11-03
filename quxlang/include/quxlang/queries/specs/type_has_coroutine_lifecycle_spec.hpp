// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_HAS_COROUTINE_LIFECYCLE_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_HAS_COROUTINE_LIFECYCLE_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/functum_list_user_overload_declarations.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/type_has_coroutine_lifecycle.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Declares queries required to inspect owned lifecycle declarations. */
    struct type_has_coroutine_lifecycle_spec
    {
        using query = type_has_coroutine_lifecycle_query;
        using dependencies = rpnx::typelist< class_type_query, functum_list_user_overload_declarations_query,
            struct_direct_bases_query, struct_field_list_query, union_info_query, variant_info_query >;
    };

    /** Traverses owned subobjects and active-alternative possibilities without following borrowed types. */
    auto type_has_coroutine_lifecycle_impl(type_symbol input) -> rpnx::querygraph::coroutine< type_has_coroutine_lifecycle_spec >;
}

#endif
