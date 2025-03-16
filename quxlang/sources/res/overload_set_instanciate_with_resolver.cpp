// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/res/function.hpp"

#include "../../../rpnx/include/rpnx/debug.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include <vector>

#include "quxlang/compiler.hpp"

using namespace quxlang;

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_ensig_initialize_with)
{
    auto os = input.ensig;
    auto args = input.params;
    // TODO: support default values for arguments

    auto val = this;


     std::string to = to_string(os.interface);
     std::string from = to_string(args);

    if (to == "INTERTYPE(MUT& MODULE(main)::buf)")
    {
        int debugger = 0;
    }

    if (os.interface.positional.size() != args.positional.size())
    {
        co_return std::nullopt;
    }

    if (os.interface.named.size() != args.named.size())
    {
        co_return std::nullopt;
    }

    std::vector< quxlang::compiler::out< bool > > convertibles_dp;


    for (auto const & [name, type] : args.named)
    {
        auto it = os.interface.named.find(name);
        if (it == os.interface.named.end())
        {
            co_return std::nullopt;
        }

        auto arg_type = type;
        auto param_type = it->second;

        std::string arg_type_str = to_string(arg_type);
        std::string param_type_str = to_string(param_type);

        // TODO: Default argument support.

        if (is_template(param_type.type))
        {
            auto match = match_template(param_type.type, arg_type);
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
            convertibles_dp.push_back(c->lk_implicitly_convertible_to(arg_type, param_type.type));
            add_co_dependency(convertibles_dp.back());
        }
    }

    for (int i = 0; i < os.interface.positional.size(); i++)
    {
        auto arg_type = args.positional.at(i);
        auto param_type = os.interface.positional.at(i);
        if (is_template(param_type.type))
        {

            QUXLANG_DEBUG(std::string arg_type_str = to_string(arg_type));
            QUXLANG_DEBUG(std::string param_type_str = to_string(param_type));

            auto tmatch = match_template(param_type.type, arg_type);
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
            convertibles_dp.push_back(c->lk_implicitly_convertible_to(arg_type, param_type.type));

            add_co_dependency(convertibles_dp.back());
        }
    }

    invotype result;

    std::optional< invotype > result_opt;

    for (auto & dp : convertibles_dp)
    {
        auto is_convertible = co_await *dp;
        if (is_convertible == false)
        {
            co_return std::nullopt;
        }
    }

    for (auto const & [name, type] : args.named)
    {
        auto it = os.interface.named.find(name);
        if (it == os.interface.named.end())
        {
            co_return std::nullopt;
        }

        auto arg_type = type;
        auto param_type = it->second;

        if (is_template(param_type.type))
        {
            auto match = match_template(param_type.type, arg_type);
            assert(match); // checked already above

            result.named[name] = std::move(match.value().type);
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

            result.named[name] = param_type.type;
        }
    }

    for (std::size_t i = 0; i < args.positional.size(); i++)
    {
        auto arg_type = args.positional[i];
        auto param_type = os.interface.positional[i];

        if (is_template(param_type.type))
        {
            auto match = match_template(param_type.type, arg_type);
            assert(match); // checked already above

            result.positional.push_back(std::move(match.value().type));
        }
        else
        {
            auto dp = convertibles_dp[i];
            auto arg_is_convertible = co_await *dp;

            if (arg_is_convertible == false)
            {
                co_return std::nullopt;
            }

            result.positional.push_back(param_type.type);
        }
    }

    result_opt = result;
    co_return result_opt;
}
