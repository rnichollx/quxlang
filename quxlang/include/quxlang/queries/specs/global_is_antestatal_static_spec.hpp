// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_GLOBAL_IS_ANTESTATAL_STATIC_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_GLOBAL_IS_ANTESTATAL_STATIC_SPEC_HEADER_GUARD

#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/class_tags.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_antestatal.hpp>
#include <quxlang/queries/variable_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct global_is_antestatal_static_spec
    {
        using query = global_is_antestatal_static_query;
        using dependencies = rpnx::typelist< class_tags_query, symboid_query, symbol_type_query, type_is_antestatal_query, variable_type_query >;
    };

    rpnx::querygraph::coroutine< global_is_antestatal_static_spec > global_is_antestatal_static_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_GLOBAL_IS_ANTESTATAL_STATIC_SPEC_HEADER_GUARD
