// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_requires_gen_move_ctor_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_requires_gen_move_ctor_spec > quxlang::class_requires_gen_move_ctor_impl(type_symbol input)
{
    if (is_atomic_type(input))
    {
        co_return false;
    }

    class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (concrete_kind == class_kind::union_)
    {
        union_info const& info = co_await rpnx::querygraph::request< union_info_query >(input);
        if (!info.properties.generate_move || (info.properties.never_valueless && !info.properties.default_index.has_value()))
        {
            co_return false;
        }
        if (!info.properties.never_valueless)
        {
            co_return true;
        }
        type_symbol const& default_type = info.options.at(*info.properties.default_index).type;
        co_return typeis< void_type >(default_type) || (co_await rpnx::querygraph::request< class_default_ctor_query >(default_type)).has_value();
    }
    if (concrete_kind == class_kind::variant)
    {
        variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(input);
        if (!info.properties.generate_move || (info.properties.never_valueless && !info.properties.default_index.has_value()))
        {
            co_return false;
        }
        if (!info.properties.never_valueless)
        {
            co_return true;
        }
        type_symbol const& default_type = info.alternatives.at(*info.properties.default_index);
        co_return typeis< void_type >(default_type) || (co_await rpnx::querygraph::request< class_default_ctor_query >(default_type)).has_value();
    }

    auto have_user_move_ctor = co_await rpnx::querygraph::request< user_move_ctor_exists_query >(input);
    if (have_user_move_ctor)
    {
        co_return false;
    }

    // auto have_nontrivial_member_ctor = co_await rpnx::querygraph::request< have_nontrivial_member_ctor_query >(input);

    co_return true; // have_nontrivial_member_ctor;
}
