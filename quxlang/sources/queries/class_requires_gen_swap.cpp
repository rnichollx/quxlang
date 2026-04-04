// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_requires_gen_swap_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/value.hpp"


rpnx::querygraph::coroutine< quxlang::class_requires_gen_swap_spec > quxlang::class_requires_gen_swap_impl(type_symbol input)
{
    auto have_required_func = co_await rpnx::querygraph::query_request< user_swap_exists_query >(input);
    if (have_required_func)
    {
        co_return false;
    }

    auto const& tags = co_await rpnx::querygraph::query_request< class_tags_query >(input);

    static std::set< std::string > const forbidden_tags = {
        "NO_BUILTIN_SWAP",
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
