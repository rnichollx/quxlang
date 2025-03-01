//
// Created by Ryan Nicholl on 2024-12-21.
//

#include "quxlang/res/constructor.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/data/expression.hpp"
#include "rpnx/value.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_default_dtor_exists)
{
    auto dtor_symbol = submember{.of = input, .name = "DESTRUCTOR"};

    auto user_defined_dtor = co_await QUX_CO_DEP(functum_overloads, (dtor_symbol));

    auto dtor_call_type = invotype{.named{{"THIS", dvalue_slot{input}}}};
    auto dtor_default_intertype = intertype{.named{{"THIS", argif{.type = dvalue_slot{input}}}}};

    // Look through destructors to find default destructor

    std::optional< type_symbol > class_default_dtor;

    for (auto& ol : user_defined_dtor)
    {
        auto candidate = co_await QUX_CO_DEP(function_ensig_initialize_with, ({.ensig = ol, .params = dtor_call_type}));

        if (candidate)
        {
            instanciation_reference inst;
            inst.temploid = temploid_reference{.templexoid = dtor_symbol, .which = ol };
            inst.params = *candidate;
            co_return true;
        }
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_default_ctor_exists)
{
    auto dtor_symbol = submember{.of = input, .name = "CONSTRUCTOR"};

    auto user_defined_dtor = co_await QUX_CO_DEP(functum_overloads, (dtor_symbol));

    auto ctor_call_type = invotype{.named{{"THIS", nvalue_slot{input}}}};
    auto ctor_default_intertype = intertype{.named{{"THIS", argif{.type = nvalue_slot{input}}}}};

    // Look through destructors to find default destructor

    std::optional< type_symbol > class_default_dtor;

    for (auto& ol : user_defined_dtor)
    {
        auto candidate = co_await QUX_CO_DEP(function_ensig_initialize_with, ({.ensig = ol, .params = ctor_call_type}));

        if (candidate)
        {
            co_return true;
        }
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_default_dtor)
{
    auto dtor_symbol = submember{.of = input, .name = "DESTRUCTOR"};

    initialization_reference init;
    init.initializee = dtor_symbol;
    init.parameters = invotype{.named{{"THIS", dvalue_slot{input}}}};

    auto dtor_inst = co_await QUX_CO_DEP(functum_initialize, (init));

    co_return dtor_inst;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_default_ctor)
{
    auto ctor_symbol = submember{.of = input, .name = "CONSTRUCTOR"};

    initialization_reference init;
    init.initializee = ctor_symbol;
    init.parameters = invotype{.named{{"THIS", nvalue_slot{input}}}};

    auto ctor_inst = co_await QUX_CO_DEP(functum_initialize, (init));

    co_return ctor_inst;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_trivially_constructible)
{
   co_return (co_await QUX_CO_DEP(class_default_ctor, (input))).has_value() == false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_trivially_destructible)
{
    co_return (co_await QUX_CO_DEP(class_default_dtor, (input))).has_value() == false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_requires_gen_default_ctor)
{
    auto have_user_default_ctor = co_await QUX_CO_DEP(user_default_ctor_exists, (input));
    if (have_user_default_ctor)
    {
        co_return false;
    }

    auto have_nontrivial_member_ctor = co_await QUX_CO_DEP(have_nontrivial_member_ctor, (input));

    co_return have_nontrivial_member_ctor;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_requires_gen_default_dtor)
{
    auto have_user_default_dtor = co_await QUX_CO_DEP(user_default_dtor_exists, (input));
    if (have_user_default_dtor)
    {
        co_return false;
    }

    auto have_nontrivial_member_dtor = co_await QUX_CO_DEP(have_nontrivial_member_dtor, (input));

    co_return have_nontrivial_member_dtor;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(have_nontrivial_member_dtor)
{
    auto class_is_builtin = co_await QUX_CO_DEP(class_builtin, (input));
    if (class_is_builtin)
    {
        co_return false;
    }

    auto class_fields = co_await QUX_CO_DEP(class_field_list, (input));
    for (auto& field : class_fields)
    {
        auto field_dtor = co_await QUX_CO_DEP(class_default_dtor, (field.type));
        if (field_dtor)
        {
            co_return true;
        }
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(have_nontrivial_member_ctor)
{
    auto class_is_builtin = co_await QUX_CO_DEP(class_builtin, (input));
    if (class_is_builtin)
    {
        co_return false;
    }

    auto class_fields = co_await QUX_CO_DEP(class_field_list, (input));
    for (auto& field : class_fields)
    {
        auto field_ctor = co_await QUX_CO_DEP(class_default_ctor, (field.type));
        if (field_ctor)
        {
            co_return true;
        }
    }

    co_return false;
}