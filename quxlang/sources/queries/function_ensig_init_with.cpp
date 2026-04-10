// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_ensig_init_with_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

rpnx::querygraph::coroutine< quxlang::function_ensig_init_with_spec > quxlang::function_ensig_init_with_impl(ensig_initialization input)
{
    auto os = input.ensig;
    auto preargs = input.params;
    // TODO: support default values for arguments

    std::string to = to_string(os.interface);
    std::string from = to_string(preargs);

    if (os.interface.positional.size() != preargs.positional.size())
    {
        // TODO: Support default arguments.
        std::cout << "Positional size mismatch: " << to << " vs " << from << "\n";
        co_return std::nullopt;
    }

    if (os.interface.named.size() != preargs.named.size())
    {
        // TODO: Support default arguments.
        co_return std::nullopt;
    }

    invotype result;

    for (auto const& [name, type] : preargs.named)
    {
        auto it = os.interface.named.find(name);
        if (it == os.interface.named.end())
        {
            co_return std::nullopt;
        }

        auto preargument_type = type;
        auto param_type = it->second;

        std::string arg_type_str = to_string(preargument_type);
        std::string param_type_str = to_string(param_type);

        auto argument_type = co_await rpnx::querygraph::request< ensig_argument_initialize_query >({.from = preargument_type, .to = param_type.type, .adaptations = input.adaptations});

        if (!argument_type)
        {
            co_return std::nullopt;
        }
        result.named[name] = *argument_type;
    }

    for (std::size_t i = 0; i < os.interface.positional.size(); i++)
    {
        auto arg_type = preargs.positional.at(i);
        auto param_type = os.interface.positional.at(i);
        auto argument_type = co_await rpnx::querygraph::request< ensig_argument_initialize_query >({.from = arg_type, .to = param_type.type, .adaptations = input.adaptations});
        if (!argument_type)
        {
            co_return std::nullopt;
        }
        result.positional.push_back(*argument_type);
    }

    std::cout << "Function ensig init with " << to_string(input.ensig.interface) << " with " << to_string(input.params) << " yields " << to_string(result) << "\n";

    co_return result;
}
