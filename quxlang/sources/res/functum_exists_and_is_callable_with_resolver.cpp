//
// Created by Ryan Nicholl on 11/16/23.
//
#include "quxlang/compiler.hpp"

#include "quxlang/res/functum_exists_and_is_callable_with_resolver.hpp"

#include "../../../rpnx/include/rpnx/debug.hpp"

std::string quxlang::functum_exists_and_is_callable_with_resolver::question() const
{
    return "functum_exists_and_is_callable_with(" + to_string(input_val.first) + ", " + to_string(input_val.second) + ")";
}

rpnx::resolver_coroutine< quxlang::compiler, bool > quxlang::functum_exists_and_is_callable_with_resolver::co_process(compiler* c, input_type input)
{

    type_symbol func = input.first;
    call_parameter_information args = input.second;
    // TODO: implement this case later
    assert(!typeis< instanciation_reference >(func));

    auto t = this;

    QUXLANG_DEBUG(std::string args_str = to_string(args));
    QUXLANG_DEBUG(std::string func_str = to_string(func));

    QUXLANG_DEBUG({if (args_str == "call_os(NUMERIC_LITERAL, MUT& T(t1))")
        {
        //std::cout << debug_recursive() << std::endl;
        int x = 0;
        }
        })

    auto overloads_opt = co_await *c->lk_list_functum_overloads(func);

    if (!overloads_opt.has_value())
    {
        co_return false;
    }

    std::set< call_parameter_information > const& overloads = overloads_opt.value();
    // TODO: Add priority/fail on ambiguity

    for (call_parameter_information const& overload : overloads)
    {
        auto is_callable = co_await *c->lk_overload_set_is_callable_with(overload, args);

        if (is_callable)
        {
            co_return true;
        }
    }

    co_return false;
}