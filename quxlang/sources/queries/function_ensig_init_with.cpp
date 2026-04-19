// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_ensig_init_with_spec.hpp>

#include "quxlang/data/temploid_instanciation_parameter_set.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

namespace
{
    /// Returns the index of the single positional pack in an interface, if one exists.
    auto positional_pack_index(quxlang::intertype const& interface) -> std::optional< std::size_t >
    {
        std::optional< std::size_t > result;
        for (std::size_t i = 0; i < interface.positional.size(); i++)
        {
            if (!interface.positional.at(i).is_pack)
            {
                if (result.has_value())
                {
                    throw std::logic_error("A positional parameter cannot follow a positional variadic pack");
                }
                continue;
            }
            if (result.has_value())
            {
                throw std::logic_error("Only one positional variadic pack is supported");
            }
            result = i;
        }
        return result;
    }

    /// Returns the formal parameter that should accept one expanded positional argument.
    auto positional_formal_for(quxlang::intertype const& interface, std::optional< std::size_t > pack_index, std::size_t argument_index) -> quxlang::argif const&
    {
        if (pack_index.has_value() && argument_index >= *pack_index)
        {
            return interface.positional.at(*pack_index);
        }
        return interface.positional.at(argument_index);
    }

    /// Merges template parameter deductions while requiring repeated named tempars to match.
    auto merge_template_matches(quxlang::temploid_instanciation_parameter_set& merged, quxlang::template_match_results const& matches) -> bool
    {
        for (auto const& [name, type] : matches.matches)
        {
            auto const existing = merged.parameter_map.find(name);
            if (existing != merged.parameter_map.end())
            {
                if (existing->second != type)
                {
                    return false;
                }
                continue;
            }
            merged.parameter_map[name] = type;
        }
        return true;
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::function_ensig_init_with_spec > quxlang::function_ensig_init_with_impl(ensig_initialization input)
{
    auto os = input.ensig;
    auto preargs = input.params;
    // TODO: support default values for arguments

    std::string to = to_string(os.interface);
    std::string from = to_string(preargs);

    auto const pack_index = positional_pack_index(os.interface);
    auto const fixed_positional_count = pack_index.value_or(os.interface.positional.size());
    if ((!pack_index.has_value() && os.interface.positional.size() != preargs.positional.size()) || (pack_index.has_value() && preargs.positional.size() < fixed_positional_count))
    {
        // TODO: Support default arguments.
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("Positional size mismatch: {} vs {}", to, from);
        }
        co_return std::nullopt;
    }

    if (os.interface.named.size() != preargs.named.size())
    {
        // TODO: Support default arguments.
        co_return std::nullopt;
    }

    instatype result;
    temploid_instanciation_parameter_set deduced_templates;

    for (auto const& [name, argument] : preargs.named)
    {
        auto it = os.interface.named.find(name);
        if (it == os.interface.named.end())
        {
            co_return std::nullopt;
        }

        if (argument.template type_is< parameter_value_instantiation >())
        {
            co_return std::nullopt;
        }

        auto preargument_type = parameter_instantiation_type(argument);
        auto param_type = it->second;
        if (param_type.is_pack)
        {
            throw std::logic_error("Named variadic packs are not supported");
        }

        std::string arg_type_str = to_string(preargument_type);
        std::string param_type_str = to_string(param_type);

        auto argument_type = co_await rpnx::querygraph::request< ensig_argument_initialize_query >({.from = preargument_type, .to = param_type.type, .adaptations = input.adaptations});

        if (!argument_type)
        {
            co_return std::nullopt;
        }
        auto match_results = match_template(param_type.type, *argument_type);
        if (!match_results.has_value() || !merge_template_matches(deduced_templates, *match_results))
        {
            co_return std::nullopt;
        }
        result.named[name] = make_type_instantiation(*argument_type);
    }

    for (std::size_t i = 0; i < preargs.positional.size(); i++)
    {
        auto const& argument = preargs.positional.at(i);
        if (argument.template type_is< parameter_value_instantiation >())
        {
            co_return std::nullopt;
        }

        auto arg_type = parameter_instantiation_type(argument);
        auto const& param_type = positional_formal_for(os.interface, pack_index, i);
        auto argument_type = co_await rpnx::querygraph::request< ensig_argument_initialize_query >({.from = arg_type, .to = param_type.type, .adaptations = input.adaptations});
        if (!argument_type)
        {
            co_return std::nullopt;
        }
        auto match_results = match_template(param_type.type, *argument_type);
        if (!match_results.has_value() || !merge_template_matches(deduced_templates, *match_results))
        {
            co_return std::nullopt;
        }
        result.positional.push_back(make_type_instantiation(*argument_type));
    }

    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        co_yield rpnx::querygraph::debug_message("Function ensig init with {} with {} yields {}", to_string(input.ensig.interface), to_string(input.params), to_string(result));
    }

    co_return result;
}
