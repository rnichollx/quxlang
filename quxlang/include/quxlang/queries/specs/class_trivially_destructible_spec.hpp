// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_DESTRUCTIBLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_DESTRUCTIBLE_SPEC_HEADER_GUARD

#include <quxlang/queries/class_trivially_destructible.hpp>
#include <quxlang/queries/class_default_dtor.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_trivially_destructible_spec
    {
        using query = class_trivially_destructible_query;
        using dependencies = rpnx::typelist< class_default_dtor_query, class_trivially_destructible_query, class_type_query, union_info_query, variant_info_query >;
    };

    rpnx::querygraph::coroutine< class_trivially_destructible_spec > class_trivially_destructible_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_DESTRUCTIBLE_SPEC_HEADER_GUARD
