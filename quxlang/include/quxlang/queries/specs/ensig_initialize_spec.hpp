// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ENSIG_INITIALIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ENSIG_INITIALIZE_SPEC_HEADER_GUARD

#include <new>

#include <rpnx/querygraph/querygraph.hpp>

#include <quxlang/queries/ensig_argument_initialize.hpp>
#include <quxlang/queries/ensig_initialize.hpp>
#include <quxlang/queries/pseudotype_match.hpp>

namespace quxlang
{
    struct ensig_initialize_spec
    {
        using query = ensig_initialize_query;
        using dependencies = rpnx::typelist< ensig_argument_initialize_query, pseudotype_match_query >;
    };

    rpnx::querygraph::coroutine< ensig_initialize_spec > ensig_initialize_impl(ensig_initialization input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ENSIG_INITIALIZE_SPEC_HEADER_GUARD
