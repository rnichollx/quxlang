// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_USER_OVERLOADS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_USER_OVERLOADS_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_user_overloads.hpp>
#include <quxlang/queries/functum_map_user_formal_ensigs.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functum_user_overloads_spec
    {
        using query = functum_user_overloads_query;
        using dependencies = rpnx::typelist< functum_map_user_formal_ensigs_query >;
    };

    rpnx::querygraph::coroutine< functum_user_overloads_spec > functum_user_overloads_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_USER_OVERLOADS_SPEC_HEADER_GUARD
