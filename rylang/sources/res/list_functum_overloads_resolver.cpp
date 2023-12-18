//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/list_functum_overloads_resolver.hpp"

#include "rylang/operators.hpp"

rpnx::resolver_coroutine< rylang::compiler, std::optional< std::set< rylang::call_parameter_information > > > rylang::list_functum_overloads_resolver::co_process(compiler* c, type_symbol functum)
{
    auto builtins = co_await *c->lk_builtin_functum_overloads(functum);
    auto user_defined = co_await *c->lk_user_functum_overloads(functum);

    std::string name = to_string(functum);
    std::set< call_parameter_information > all_overloads;
    for (auto const& o : builtins)
    {
        all_overloads.insert(o);
    }
    for (auto const& o : user_defined)
    {
        all_overloads.insert(o);
    }

    co_return all_overloads;

}
