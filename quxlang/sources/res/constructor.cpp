//
// Created by Ryan Nicholl on 2024-12-21.
//

#include "quxlang/res/constructor.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/data/expression.hpp"
#include "rpnx/value.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(nontrivial_default_dtor)
{
    // Check if user has defined a destructor
    auto dtor_symbol = submember{.of = input, .name = "DESTRUCTOR"};

    auto user_defined_dtor = co_await QUX_CO_DEP(list_user_functum_overloads, (dtor_symbol));

    auto dtor_call_type = calltype{.named{{"THIS", dvalue_slot{input}}}};

    // Look through destructors to find default destructor

    std::optional< type_symbol > default_dtor;

    for (auto& ol : user_defined_dtor)
    {
        auto candidate = co_await QUX_CO_DEP(overload_set_instanciate_with, ({.overload = ol, .call = dtor_call_type}));

        if (candidate)
        {
            auto dtor_selection = selection_reference{.templexoid = input, .overload = ol};
            auto dtor_instanciation = instantiation_type{.callee = dtor_selection, .parameters = *candidate};

            co_return dtor_instanciation;
        }
    }

    auto kind = co_await QUX_CO_DEP(symbol_type, (input));

    if (kind == symbol_kind::user_class)
    {

        // If no user defined destructor, we need to check if there is a non-trivial default destructor
        // The default destructor is non-trivial if the destructor of any member is non-trivial

        auto member_list = co_await QUX_CO_DEP(class_field_list, (input));

        for (auto& member_fld : member_list)
        {
            auto member_has_nontrivial_dtor = co_await QUX_CO_DEP(nontrivial_default_dtor, (member_fld.type));

            if (member_has_nontrivial_dtor)
            {
                // We need a built-in destructor
                auto builtin_dtor_selection = selection_reference{.templexoid = input, .overload = {.builtin = true, .call_parameters = dtor_call_type}};

                auto builtin_dtor_instanciation = instantiation_type{.callee = builtin_dtor_selection, .parameters = dtor_call_type, };

                co_return builtin_dtor_instanciation;
            }
        }

        // TODO: Inheritance?
    }

    co_return std::nullopt;
}