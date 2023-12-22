//
// Created by Ryan Nicholl on 10/28/23.
//

#include "rylang/res/overload_set_is_callable_with_resolver.hpp"

#include "rylang/debug.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include <vector>

using namespace rylang;

rpnx::resolver_coroutine< compiler, bool > rylang::overload_set_is_callable_with_resolver::co_process(compiler* c, input_type input)
{
    auto os = input.first;
    auto args = input.second;

    std::string mos_str = to_string(os);
    std::string args_str = to_string(args);

    if (args_str == "call_os(MUT& T(t1))")
    {
        int x = 0;
    }

    auto result = co_await *c->lk_overload_set_instanciate_with(os, args);

    auto result_str = result.has_value() ? to_string(result.value()) : "nullopt";

    if (args_str == "call_os(MUT& T(t1))")
    {
        //std::cout << debug_recursive() << std::endl;
        int x = 0;
    }

    co_return result.has_value();
}
