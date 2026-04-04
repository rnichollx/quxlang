// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_requires_gen_default_dtor_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/value.hpp"


rpnx::querygraph::coroutine< quxlang::class_requires_gen_default_dtor_spec > quxlang::class_requires_gen_default_dtor_impl(type_symbol input)
{
    auto have_user_default_dtor = co_await rpnx::querygraph::query_request< user_default_dtor_exists_query >(input);
    if (have_user_default_dtor)
    {
        co_return false;
    }

    auto have_nontrivial_member_dtor = co_await rpnx::querygraph::query_request< have_nontrivial_member_dtor_query >(input);

    co_return have_nontrivial_member_dtor;
}
