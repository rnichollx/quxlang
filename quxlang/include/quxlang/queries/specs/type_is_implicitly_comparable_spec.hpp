// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_IMPLICITLY_COMPARABLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_IMPLICITLY_COMPARABLE_SPEC_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/fusion_alternatives_list.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_implicitly_comparable.hpp>
#include <quxlang/queries/type_is_implicitly_datatype.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Declares dependencies for structural comparison eligibility. */
    struct type_is_implicitly_comparable_spec
    {
        using query = type_is_implicitly_comparable_query;
        using dependencies = rpnx::typelist< class_type_query, fusion_alternatives_list_query, struct_direct_bases_query,
            struct_field_list_query, struct_tags_query, symbol_type_query, type_is_implicitly_datatype_query >;
    };

    /** Checks stored type graphs, including recursive boxed alternatives, for structural comparison eligibility. */
    rpnx::querygraph::coroutine< type_is_implicitly_comparable_spec > type_is_implicitly_comparable_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_IS_IMPLICITLY_COMPARABLE_SPEC_HEADER_GUARD
