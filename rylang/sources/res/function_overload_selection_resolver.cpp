//
// Created by Ryan Nicholl on 11/4/23.
//

#include "rylang/compiler.hpp"

#include "rylang/manipulators/qmanip.hpp"

#include <iostream>
#include <sstream>

void rylang::function_overload_selection_resolver::process(compiler* c)
{
    std::stringstream ss;
    std::cout << "Function overload selection resolver called for " << to_string(m_function_location) << std::endl;
    std::cout << "With args:" << std::endl;
    auto args = m_args;

    for (auto & argtype : args.argument_types)
    {
       std::cout << " " << to_string(argtype) << ",";
    }

    std::cout << std::endl;

    auto overloads_dp = get_dependency(
        [&]
        {
            return c->lk_list_functum_overloads(m_function_location);
        });

    if (!ready())
    {
        return;
    }

    auto overloads_opt = overloads_dp->get();

    if (!overloads_opt.has_value())
    {
        throw std::runtime_error("No overloads");
    }

    std::size_t eligible_overloads = 0;
    std::optional< call_parameter_information > output_overload;

    std::set< call_parameter_information > const& overloads = overloads_opt.value();

    for (call_parameter_information const& overload : overloads)
    {
        auto is_callable_dp = get_dependency(
            [&]
            {
                return c->lk_overload_set_is_callable_with(std::make_pair(overload, m_args));
            });

        if (!ready())
            return;

        bool is_callable = is_callable_dp->get();

        ss << "Checking overload " << to_string(overload) << " callable with " << to_string(m_args) << "? " << std::boolalpha << is_callable << std::endl;

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

    set_value(output_overload.value());
}
