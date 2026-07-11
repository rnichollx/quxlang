// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_TRIVIALLY_RELOCATABLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_TRIVIALLY_RELOCATABLE_SPEC_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_trivially_relocatable.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_is_trivially_relocatable_spec
    {
        using query = type_is_trivially_relocatable_query;
        using dependencies = rpnx::typelist< class_type_query, symbol_type_query, type_is_trivially_relocatable_query >;
    };

    /// Returns true when relocation is equivalent to copying the type's storage bytes.
    rpnx::querygraph::coroutine< type_is_trivially_relocatable_spec > type_is_trivially_relocatable_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_IS_TRIVIALLY_RELOCATABLE_SPEC_HEADER_GUARD
