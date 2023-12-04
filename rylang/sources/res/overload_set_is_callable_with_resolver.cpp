//
// Created by Ryan Nicholl on 10/28/23.
//

#include "rylang/res/overload_set_is_callable_with_resolver.hpp"

#include "rylang/manipulators/qmanip.hpp"
#include <vector>

using namespace rylang;

rpnx::resolver_coroutine< compiler, bool > rylang::overload_set_is_callable_with_resolver::co_process(compiler* c, input_type input)
{
    auto os = input.first;
    auto args = input.second;
    // TODO: support default values for arguments

    auto val = this;


    std::string to = to_string(os);
    std::string from = to_string(args);

    if (os.argument_types.size() != args.argument_types.size())
    {
        co_return false;
    }

    std::vector< rylang::compiler::out< bool > > convertibles_dp;

    for (int i = 0; i < os.argument_types.size(); i++)
    {

        convertibles_dp.push_back(c->lk_canonical_type_is_implicitly_convertible_to(std::make_pair(args.argument_types[i], os.argument_types[i])));

        add_dependency(convertibles_dp.back());
    }

    for (auto & dp : convertibles_dp)
    {
        auto arg_is_convertible = co_await *dp;
        if (arg_is_convertible == false)
        {
            co_return false;
        }
    }

    co_return true;
}
