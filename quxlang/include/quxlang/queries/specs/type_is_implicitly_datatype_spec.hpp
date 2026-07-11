// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_IMPLICITLY_DATATYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_IMPLICITLY_DATATYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/type_is_implicitly_datatype.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_is_implicitly_datatype_spec
    {
        using query = type_is_implicitly_datatype_query;
        using dependencies = rpnx::typelist< struct_field_list_query, class_type_query, symbol_type_query, type_is_implicitly_datatype_query >;
    };

    rpnx::querygraph::coroutine< type_is_implicitly_datatype_spec > type_is_implicitly_datatype_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_IS_IMPLICITLY_DATATYPE_SPEC_HEADER_GUARD
