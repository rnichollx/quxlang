// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/have_nontrivial_member_ctor_spec.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"

#include <optional>
#include <vector>


rpnx::querygraph::coroutine< quxlang::have_nontrivial_member_ctor_spec > quxlang::have_nontrivial_member_ctor_impl(type_symbol input)
{
    auto class_is_builtin = co_await rpnx::querygraph::request< class_builtin_query >(input);
    if (class_is_builtin)
    {
        if (typeis< array_type >(input))
        {
            auto element_type = input.get_as< array_type >().element_type;

            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("Checking nontrivial member ctor for array element type: {}", quxlang::to_string(element_type));
            }

            co_return co_await rpnx::querygraph::request< have_nontrivial_member_ctor_query >(element_type);
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
        auto field_ctor = co_await rpnx::querygraph::request< class_default_ctor_query >(field_type);
        if (field_ctor)
        {
            co_return true;
        }
    }

    std::vector< struct_base_declaration > const direct_bases = co_await rpnx::querygraph::request< struct_direct_bases_query >(input);
    for (struct_base_declaration const& base : direct_bases)
    {
        std::optional< type_symbol > const base_constructor = co_await rpnx::querygraph::request< class_default_ctor_query >(base.base_type);
        if (base_constructor.has_value())
        {
            co_return true;
        }
    }

    co_return false;
}
