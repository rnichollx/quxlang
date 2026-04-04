// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/have_nontrivial_member_dtor_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/value.hpp"


rpnx::querygraph::coroutine< quxlang::have_nontrivial_member_dtor_spec > quxlang::have_nontrivial_member_dtor_impl(type_symbol input)
{
    auto class_is_builtin = co_await rpnx::querygraph::query_request< class_builtin_query >(input);
    if (class_is_builtin)
    {
        if (typeis< array_type >(input))
        {
            co_return !co_await rpnx::querygraph::query_request< class_trivially_destructible_query >(input.get_as< array_type >().element_type);
        }
        co_return false;
    }

    auto class_fields = co_await rpnx::querygraph::query_request< class_field_list_query >(input);
    for (auto& field : class_fields)
    {
        auto field_dtor = co_await rpnx::querygraph::query_request< class_default_dtor_query >(field.type);
        if (field_dtor)
        {
            co_return true;
        }
    }

    co_return false;
}
