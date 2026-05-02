// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTANOID_SIGTYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTANOID_SIGTYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/functanoid_sigtype.hpp>
#include <quxlang/queries/functanoid_return_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functanoid_sigtype_spec
    {
        using query = functanoid_sigtype_query;
        using dependencies = rpnx::typelist< functanoid_return_type_query >;
    };

    rpnx::querygraph::coroutine< functanoid_sigtype_spec > functanoid_sigtype_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTANOID_SIGTYPE_SPEC_HEADER_GUARD
