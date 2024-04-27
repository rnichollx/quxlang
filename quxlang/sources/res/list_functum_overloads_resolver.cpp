//
// Created by Ryan Nicholl on 11/18/23.
//
#include "quxlang/compiler.hpp"

#include "quxlang/res/list_functum_overloads_resolver.hpp"

#include "quxlang/operators.hpp"



QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_functum_overloads)
{
    auto builtins = co_await *c->lk_list_builtin_functum_overloads(input);
    auto user_defined = co_await *c->lk_list_user_functum_overloads(input);

    std::string name = to_string(input);
    std::set< function_overload > all_overloads;
    for (auto const& o : builtins)
    {
        all_overloads.insert(o.overload);
    }
    for (auto const& o : user_defined)
    {
        all_overloads.insert(o);
    }

    co_return all_overloads;

}
