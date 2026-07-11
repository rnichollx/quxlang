// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/have_nontrivial_member_dtor_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::have_nontrivial_member_dtor_spec > quxlang::have_nontrivial_member_dtor_impl(type_symbol input)
{
    auto class_is_builtin = co_await rpnx::querygraph::request< class_builtin_query >(input);
    if (class_is_builtin)
    {
        if (typeis< array_type >(input))
        {
            co_return !co_await rpnx::querygraph::request< class_trivially_destructible_query >(input.get_as< array_type >().element_type);
        }
        co_return false;
    }

    auto struct_fields = co_await rpnx::querygraph::request< struct_field_list_query >(input);
    for (auto& field : struct_fields)
    {
        type_symbol field_type = field.type;
        if (typeis< attached_type_reference >(field_type))
        {
            attached_type_reference const& attached = as< attached_type_reference >(field_type);
            if (typeis< void_type >(attached.carrying_type))
            {
                continue;
            }
            field_type = attached.carrying_type;
        }
        auto field_dtor = co_await rpnx::querygraph::request< class_default_dtor_query >(field_type);
        if (field_dtor)
        {
            co_return true;
        }
    }

    co_return false;
}
