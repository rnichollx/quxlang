//
// Created by Ryan Nicholl on 10/28/23.
//

#include "rylang/res/overload_set_instanciate_with_resolver.hpp"

#include "rylang/debug.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include <vector>

using namespace rylang;

rpnx::resolver_coroutine< compiler, std::optional< call_parameter_information > > rylang::overload_set_instanciate_with_resolver::co_process(compiler* c, input_type input)
{
    auto os = input.first;
    auto args = input.second;
    // TODO: support default values for arguments

    auto val = this;

    std::string to = to_string(os);
    std::string from = to_string(args);

    if (to == "call_os(MUT& T(t1))")
    {
        int x = 0;
    }

    if (os.argument_types.size() != args.argument_types.size())
    {
        co_return std::nullopt;
    }

    std::vector< rylang::compiler::out< bool > > convertibles_dp;

    for (int i = 0; i < os.argument_types.size(); i++)
    {
        auto arg_type = args.argument_types[i];
        auto param_type = os.argument_types[i];
        if (is_template(param_type))
        {

            RYLANG_DEBUG(std::string arg_type_str = to_string(arg_type));
            RYLANG_DEBUG(std::string param_type_str = to_string(param_type));

            auto tmatch = match_template(param_type, arg_type);
            if (tmatch.has_value())
            {
                convertibles_dp.push_back(nullptr);
                continue;
            }
            else
            {
                co_return std::nullopt;
            }
        }
        else
        {
            convertibles_dp.push_back(c->lk_canonical_type_is_implicitly_convertible_to(std::make_pair(arg_type, param_type)));

            add_dependency(convertibles_dp.back());
        }
    }


    call_parameter_information result;

    std::optional< call_parameter_information > result_opt;

    for (std::size_t i = 0; i < args.argument_types.size(); i++)
    {
        auto arg_type = args.argument_types[i];
        auto param_type = os.argument_types[i];

        if (is_template(param_type))
        {
            auto match = match_template(param_type, arg_type);
            assert(match); // checked already above

            result.argument_types.push_back(std::move(match.value().type));
        }
        else
        {
            auto dp = convertibles_dp[i];
            auto arg_is_convertible = co_await *dp;

            if (arg_is_convertible == false)
            {
                co_return std::nullopt;
            }

            result.argument_types.push_back(param_type);
        }
    }

    //std::cout << debug_recursive() << std::endl;

    result_opt = result;
    co_return result_opt;
}
