//
// Created by Ryan Nicholl on 10/28/23.
//

#include "quxlang/res/overload_set_instanciate_with_resolver.hpp"

#include "../../../rpnx/include/rpnx/debug.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include <vector>

#include "quxlang/compiler.hpp"

using namespace quxlang;

QUX_CO_RESOLVER_IMPL_FUNC_DEF(overload_set_instanciate_with)
{
    auto os = input.overload;
    auto args = input.call;
    // TODO: support default values for arguments

    auto val = this;

    //  std::string to = to_string(os);
    //  std::string from = to_string(args);


    if (os.call_parameters.positional_parameters.size() != args.positional_parameters.size())
    {
        co_return std::nullopt;
    }

    if (os.call_parameters.named_parameters.size() != args.named_parameters.size())
    {
        co_return std::nullopt;
    }

    std::vector< quxlang::compiler::out< bool > > convertibles_dp;


    for (auto const & [name, type] : args.named_parameters)
    {
        auto it = os.call_parameters.named_parameters.find(name);
        if (it == os.call_parameters.named_parameters.end())
        {
            co_return std::nullopt;
        }

        auto arg_type = type;
        auto param_type = it->second;

        if (is_template(param_type))
        {
            auto match = match_template(param_type, arg_type);
            if (match.has_value())
            {
                continue;
            }
            else
            {
                co_return std::nullopt;
            }
        }
        else
        {
            convertibles_dp.push_back(c->lk_implicitly_convertible_to(arg_type, param_type));
            add_co_dependency(convertibles_dp.back());
        }
    }

    for (int i = 0; i < os.call_parameters.positional_parameters.size(); i++)
    {
        auto arg_type = args.positional_parameters.at(i);
        auto param_type = os.call_parameters.positional_parameters.at(i);
        if (is_template(param_type))
        {

            QUXLANG_DEBUG(std::string arg_type_str = to_string(arg_type));
            QUXLANG_DEBUG(std::string param_type_str = to_string(param_type));

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
            convertibles_dp.push_back(c->lk_implicitly_convertible_to(arg_type, param_type));

            add_co_dependency(convertibles_dp.back());
        }
    }

    call_type result;

    std::optional< call_type > result_opt;

    for (auto & dp : convertibles_dp)
    {
        auto is_convertible = co_await *dp;
        if (is_convertible == false)
        {
            co_return std::nullopt;
        }
    }

    for (auto const & [name, type] : args.named_parameters)
    {
        auto it = os.call_parameters.named_parameters.find(name);
        if (it == os.call_parameters.named_parameters.end())
        {
            co_return std::nullopt;
        }

        auto arg_type = type;
        auto param_type = it->second;

        if (is_template(param_type))
        {
            auto match = match_template(param_type, arg_type);
            assert(match); // checked already above

            result.named_parameters[name] = std::move(match.value().type);
        }
        else
        {
            auto dp = convertibles_dp.back();
            convertibles_dp.pop_back();
            auto arg_is_convertible = co_await *dp;

            if (arg_is_convertible == false)
            {
                co_return std::nullopt;
            }

            result.named_parameters[name] = param_type;
        }
    }

    for (std::size_t i = 0; i < args.positional_parameters.size(); i++)
    {
        auto arg_type = args.positional_parameters[i];
        auto param_type = os.call_parameters.positional_parameters[i];

        if (is_template(param_type))
        {
            auto match = match_template(param_type, arg_type);
            assert(match); // checked already above

            result.positional_parameters.push_back(std::move(match.value().type));
        }
        else
        {
            auto dp = convertibles_dp[i];
            auto arg_is_convertible = co_await *dp;

            if (arg_is_convertible == false)
            {
                co_return std::nullopt;
            }

            result.positional_parameters.push_back(param_type);
        }
    }

    result_opt = result;
    co_return result_opt;
}
