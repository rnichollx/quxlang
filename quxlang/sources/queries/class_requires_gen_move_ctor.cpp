// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_requires_gen_move_ctor_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_requires_gen_move_ctor_spec > quxlang::class_requires_gen_move_ctor_impl(type_symbol input)
{
    auto have_user_move_ctor = co_await rpnx::querygraph::request< user_move_ctor_exists_query >(input);
    if (have_user_move_ctor)
    {
        co_return false;
    }

    // auto have_nontrivial_member_ctor = co_await rpnx::querygraph::request< have_nontrivial_member_ctor_query >(input);

    co_return true; // have_nontrivial_member_ctor;
}
