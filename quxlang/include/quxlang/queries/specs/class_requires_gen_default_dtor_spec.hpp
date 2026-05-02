// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_DEFAULT_DTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_DEFAULT_DTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/class_requires_gen_default_dtor.hpp>
#include <quxlang/queries/have_nontrivial_member_dtor.hpp>
#include <quxlang/queries/user_default_dtor_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_requires_gen_default_dtor_spec
    {
        using query = class_requires_gen_default_dtor_query;
        using dependencies = rpnx::typelist< have_nontrivial_member_dtor_query, user_default_dtor_exists_query >;
    };

    rpnx::querygraph::coroutine< class_requires_gen_default_dtor_spec > class_requires_gen_default_dtor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_DEFAULT_DTOR_SPEC_HEADER_GUARD
