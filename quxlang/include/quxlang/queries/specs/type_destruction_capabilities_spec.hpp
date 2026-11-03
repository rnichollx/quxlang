// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_DESTRUCTION_CAPABILITIES_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_DESTRUCTION_CAPABILITIES_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/functum_list_user_overload_declarations.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/type_destruction_capabilities.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/user_default_dtor_exists.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Declares the ownership and destructor dependencies of lifetime capability classification. */
    struct type_destruction_capabilities_spec
    {
        using query = type_destruction_capabilities_query;
        using dependencies = rpnx::typelist< class_type_query, functum_list_user_overload_declarations_query, struct_direct_bases_query, struct_field_list_query, union_info_query, user_default_dtor_exists_query, variant_info_query >;
    };

    /** Traverses owning type edges, including cycles introduced by boxed fusion alternatives. */
    rpnx::querygraph::coroutine< type_destruction_capabilities_spec > type_destruction_capabilities_impl(type_symbol input);
}

#endif
