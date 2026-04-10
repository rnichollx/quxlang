// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_requires_gen_copy_ctor_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_requires_gen_copy_ctor_spec > quxlang::class_requires_gen_copy_ctor_impl(type_symbol input)
{
    auto have_user_copy_ctor = co_await rpnx::querygraph::request< user_copy_ctor_exists_query >(input);
    if (have_user_copy_ctor)
    {
        co_return false;
    }

    auto const& tags = co_await rpnx::querygraph::request< class_tags_query >(input);

    static std::set< std::string > const forbidden_tags = {
        "NOT_COPYABLE",
        "NO_BUILTIN_COPY",
        "NO_IMPLICIT_CONSTRUCTORS",
        "MOVE_ONLY",
    };

    for (auto const& tag : forbidden_tags)
    {
        if (tags.contains(tag))
        {
            co_return false;
        }
    }

    co_return true; // have_nontrivial_member_ctor;
}
