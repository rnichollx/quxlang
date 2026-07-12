// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_requires_gen_assignment_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_requires_gen_assignment_spec > quxlang::class_requires_gen_assignment_impl(type_symbol input)
{
    if (is_atomic_type(input))
    {
        co_return false;
    }

    class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (concrete_kind == class_kind::union_)
    {
        co_return (co_await rpnx::querygraph::request< union_info_query >(input)).properties.generate_assignment;
    }
    if (concrete_kind == class_kind::variant)
    {
        co_return (co_await rpnx::querygraph::request< variant_info_query >(input)).properties.generate_assignment;
    }

    auto have_user_assignment = co_await rpnx::querygraph::request< user_assignment_exists_query >(input);
    if (have_user_assignment)
    {
        co_return false;
    }

    auto const& tags = co_await rpnx::querygraph::request< struct_tags_query >(input);

    static std::set< std::string > const forbidden_tags = {
        "NO_IMPLICIT_ASSIGNMENT",
        "NOT_COPYABLE",
    };

    for (auto const& tag : forbidden_tags)
    {
        if (tags.contains(tag))
        {
            co_return false;
        }
    }

    // auto have_nontrivial_member_ctor = co_await rpnx::querygraph::request< have_nontrivial_member_ctor_query >(input);

    co_return true; // have_nontrivial_member_ctor;
}
