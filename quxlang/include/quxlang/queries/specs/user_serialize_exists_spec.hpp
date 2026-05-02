// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_USER_SERIALIZE_EXISTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_USER_SERIALIZE_EXISTS_SPEC_HEADER_GUARD

#include <quxlang/queries/user_serialize_exists.hpp>
#include <quxlang/queries/functum_user_overloads.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct user_serialize_exists_spec
    {
        using query = user_serialize_exists_query;
        using dependencies = rpnx::typelist< functum_user_overloads_query >;
    };

    rpnx::querygraph::coroutine< user_serialize_exists_spec > user_serialize_exists_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_USER_SERIALIZE_EXISTS_SPEC_HEADER_GUARD
