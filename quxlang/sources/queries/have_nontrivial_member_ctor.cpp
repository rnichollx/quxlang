// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/have_nontrivial_member_ctor_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::have_nontrivial_member_ctor_spec > quxlang::have_nontrivial_member_ctor_impl(type_symbol input)
{
    auto class_is_builtin = co_await rpnx::querygraph::request< class_builtin_query >(input);
    if (class_is_builtin)
    {
        if (typeis< array_type >(input))
        {
            auto element_type = input.get_as< array_type >().element_type;

            std::cout << "Checking nontrivial member ctor for array element type: " << quxlang::to_string(element_type) << std::endl;

            co_return co_await rpnx::querygraph::request< have_nontrivial_member_ctor_query >(element_type);
        }
        co_return false;
    }

    auto class_fields = co_await rpnx::querygraph::request< class_field_list_query >(input);
    for (auto& field : class_fields)
    {
        auto field_ctor = co_await rpnx::querygraph::request< class_default_ctor_query >(field.type);
        if (field_ctor)
        {
            co_return true;
        }
    }

    co_return false;
}
