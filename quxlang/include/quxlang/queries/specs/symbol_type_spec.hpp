// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SYMBOL_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SYMBOL_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/functum_overloads.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/templex_builtins.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct symbol_type_spec
    {
        using query = symbol_type_query;
        using dependencies = rpnx::typelist< enum_info_query, flagset_info_query, functum_overloads_query, symboid_query, symbol_type_query, templex_builtins_query >;
    };

    rpnx::querygraph::coroutine< symbol_type_spec > symbol_type_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SYMBOL_TYPE_SPEC_HEADER_GUARD
