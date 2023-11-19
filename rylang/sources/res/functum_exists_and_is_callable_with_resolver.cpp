//
// Created by Ryan Nicholl on 11/16/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/functum_exists_and_is_callable_with_resolver.hpp"


void rylang::functum_exists_and_is_callable_with_resolver::process(compiler* c)
{
    // TODO: implement this case later
    assert(!typeis< functanoid_reference >(func));

    auto overloads_dp = get_dependency(
        [&]
        {
            return c->lk_list_functum_overloads(func);
        });

    if (!ready())
    {
        return;
    }

    auto overloads_opt = overloads_dp->get();

    if (!overloads_opt.has_value())
    {
        set_value(false);
        return;
    }

    std::vector<call_parameter_information> const & overloads = overloads_opt.value();
    // TODO: Add priority/fail on ambiguity

    for (call_parameter_information const& overload : overloads)
    {
        auto is_callable_dp = get_dependency(
            [&]
            {
                return c->lk_overload_set_is_callable_with(overload, args);
            });
        if (!ready())
        {
            return;
        }
        bool is_callable = is_callable_dp->get();
        if (is_callable)
        {
            set_value(true);
            return;
        }
    }

    set_value(false);
}
