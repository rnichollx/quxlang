// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_requires_gen_swap_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/compilation_result.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_requires_gen_swap_spec > quxlang::class_requires_gen_swap_impl(type_symbol input)
{
    class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (concrete_kind == class_kind::union_)
    {
        co_return (co_await rpnx::querygraph::request< union_info_query >(input)).properties.generate_swap;
    }
    if (concrete_kind == class_kind::variant)
    {
        co_return (co_await rpnx::querygraph::request< variant_info_query >(input)).properties.generate_swap;
    }

    struct_tags_result_type const& tags = co_await rpnx::querygraph::request< struct_tags_query >(input);
    bool have_required_func = co_await rpnx::querygraph::request< user_swap_exists_query >(input);
    if (tags.contains(keywords::polymorphic) || tags.contains(keywords::virtual_polymorphic))
    {
        co_return false;
    }
    if (tags.contains(keywords::rooted))
    {
        std::set< temploid_ensig > const& user_rhs_swap = co_await rpnx::querygraph::request< functum_user_overloads_query >(submember{
            .of = input,
            .name = "OPERATOR<->RHS",
        });
        if (have_required_func || !user_rhs_swap.empty())
        {
            throw semantic_compilation_error("ROOTED structs cannot declare a swap operator");
        }
        co_return false;
    }
    if (have_required_func)
    {
        co_return false;
    }

    co_return !tags.contains(keywords::no_default_swap);
}
