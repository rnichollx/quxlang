// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLOID_FORMAL_ENSIG_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLOID_FORMAL_ENSIG_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_builtin_overloads.hpp>
#include <quxlang/queries/functum_map_user_formal_ensigs.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/templex_builtins.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct temploid_formal_ensig_spec
    {
        using query = temploid_formal_ensig_query;
        using dependencies = rpnx::typelist<
            functum_builtin_overloads_query,
            functum_map_user_formal_ensigs_query,
            lookup_query,
            symboid_query,
            symbol_type_query,
            templex_builtins_query
        >;
    };

    rpnx::querygraph::coroutine< temploid_formal_ensig_spec > temploid_formal_ensig_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLOID_FORMAL_ENSIG_SPEC_HEADER_GUARD
