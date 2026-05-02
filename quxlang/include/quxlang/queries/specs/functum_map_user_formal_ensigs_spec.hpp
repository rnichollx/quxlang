// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_MAP_USER_FORMAL_ENSIGS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_MAP_USER_FORMAL_ENSIGS_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_map_user_formal_ensigs.hpp>
#include <quxlang/queries/functum_list_user_ensig_declarations.hpp>
#include <quxlang/queries/lookup.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functum_map_user_formal_ensigs_spec
    {
        using query = functum_map_user_formal_ensigs_query;
        using dependencies = rpnx::typelist< functum_list_user_ensig_declarations_query, lookup_query >;
    };

    rpnx::querygraph::coroutine< functum_map_user_formal_ensigs_spec > functum_map_user_formal_ensigs_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_MAP_USER_FORMAL_ENSIGS_SPEC_HEADER_GUARD
