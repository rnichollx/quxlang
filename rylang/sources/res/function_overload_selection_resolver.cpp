//
// Created by Ryan Nicholl on 11/4/23.
//

#include "rylang/compiler.hpp"

#include "rylang/manipulators/qmanip.hpp"

#include <iostream>
#include <sstream>

rpnx::resolver_coroutine<rylang::compiler, rylang::call_parameter_information> rylang::function_overload_selection_resolver::co_process(compiler* c, key_type input)
{

    std::stringstream ss;

    auto funcloc = input.first;
    ss << "Function overload selection resolver called for " << to_string(funcloc) << std::endl;
    ss << "With args:" << std::endl;
    auto args = input.second;



    for (auto & argtype : args.argument_types)
    {
       ss << " " << to_string(argtype) << ",";
    }

    ss << std::endl;

    auto overloads_opt  = co_await *c->lk_list_functum_overloads(funcloc);


    if (!overloads_opt.has_value())
    {
        throw std::runtime_error("No overloads");
    }

    std::size_t eligible_overloads = 0;
    std::optional< call_parameter_information > output_overload;

    std::set< call_parameter_information > const& overloads = overloads_opt.value();

    for (call_parameter_information const& overload : overloads)
    {
        auto is_callable = co_await* c->lk_overload_set_is_callable_with(std::make_pair(overload, args));

        ss << "Checking overload " << to_string(overload) << " callable with " << to_string(args) << "? " << std::boolalpha << is_callable << std::endl;

        if (is_callable)
        {
            eligible_overloads++;
            output_overload = overload;
        }
    }

    // TODO: Remove this
    std::cout << ss.str() << std::endl;

    if (eligible_overloads == 0)
    {

        throw std::runtime_error("No eligible overloads");
    }
    else if (eligible_overloads > 1)
    {
        throw std::runtime_error("Ambiguous overload");
    }

    co_return output_overload.value();
}
