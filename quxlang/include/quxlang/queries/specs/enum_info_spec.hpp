// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ENUM_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ENUM_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct enum_info_spec
    {
        using query = enum_info_query;
        using dependencies = rpnx::typelist< constexpr_u64_query, symboid_query >;
    };

    rpnx::querygraph::coroutine< enum_info_spec > enum_info_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ENUM_INFO_SPEC_HEADER_GUARD
