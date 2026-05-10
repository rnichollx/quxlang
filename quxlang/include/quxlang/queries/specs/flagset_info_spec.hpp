// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FLAGSET_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FLAGSET_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct flagset_info_spec
    {
        using query = flagset_info_query;
        using dependencies = rpnx::typelist< constexpr_u64_query, symboid_query >;
    };

    rpnx::querygraph::coroutine< flagset_info_spec > flagset_info_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FLAGSET_INFO_SPEC_HEADER_GUARD
