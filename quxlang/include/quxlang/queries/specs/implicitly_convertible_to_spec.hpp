// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_IMPLICITLY_CONVERTIBLE_TO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_IMPLICITLY_CONVERTIBLE_TO_SPEC_HEADER_GUARD

#include <quxlang/queries/implicitly_convertible_to.hpp>
#include <quxlang/queries/ensig_argument_initialize.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct implicitly_convertible_to_spec
    {
        using query = implicitly_convertible_to_qg_query;
        using dependencies = rpnx::typelist< ensig_argument_initialize_query >;
    };

    rpnx::querygraph::coroutine< implicitly_convertible_to_spec > implicitly_convertible_to_impl(implicitly_convertible_to_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_IMPLICITLY_CONVERTIBLE_TO_SPEC_HEADER_GUARD
