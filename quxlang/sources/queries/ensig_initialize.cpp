// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/ensig_initialize_spec.hpp>

#include <quxlang/data/temploid_instanciation_parameter_set.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/manipulators/typeutils.hpp>

rpnx::querygraph::coroutine< quxlang::ensig_initialize_spec > quxlang::ensig_initialize_impl(ensig_initialization input)
{
    auto const& ensig = input.ensig;
    auto const& preargs = input.params;

    auto positional_pack_index = [](intertype const& interface) -> std::optional< std::size_t >
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
    };

    auto positional_formal_for = [](intertype const& interface, std::optional< std::size_t > pack_index, std::size_t argument_index) -> argif const&
    {
        if (pack_index.has_value() && argument_index >= *pack_index)
        {
            return interface.positional.at(*pack_index);
        }
        return interface.positional.at(argument_index);
    };

    auto merge_template_matches = [](temploid_instanciation_parameter_set& merged, template_match_results const& matches) -> bool
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
    };

    auto const pack_index = positional_pack_index(ensig.interface);
    auto const fixed_positional_count = pack_index.value_or(ensig.interface.positional.size());
    if ((!pack_index.has_value() && preargs.positional.size() > ensig.interface.positional.size()) || (pack_index.has_value() && preargs.positional.size() < fixed_positional_count))
    {
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("Positional size mismatch: {} vs {}", to_string(ensig.interface), to_string(preargs));
        }
        co_return std::nullopt;
    }

    instatype result;
    temploid_instanciation_parameter_set deduced_templates;

    auto initialize_argument_type =
        [&](type_symbol const& from, type_symbol const& to) -> rpnx::querygraph::coroutine< ensig_initialize_spec >::cosubroutine< std::optional< type_symbol > >
    {
        if (input.adaptations == allowed_adaptations::none)
        {
            if (match_template(to, from).has_value())
            {
                co_return from;
            }
            co_return std::nullopt;
        }

        co_return co_await rpnx::querygraph::request< ensig_argument_initialize_query >(argument_init_input{
            .from = from,
            .to = to,
            .adaptations = input.adaptations,
        });
    };

    auto initialize_parameter =
        [&](argif const& formal, parameter_instantiation const& actual) -> rpnx::querygraph::coroutine< ensig_initialize_spec >::cosubroutine< std::optional< parameter_instantiation > >
    {
        auto const actual_is_value = actual.template type_is< parameter_value_instantiation >();
        if (formal.requires_static_value != actual_is_value)
        {
            co_return std::nullopt;
        }

        auto const actual_type = parameter_instantiation_type(actual);
        auto initialized_type = co_await initialize_argument_type(actual_type, formal.type);
        if (!initialized_type.has_value())
        {
            co_return std::nullopt;
        }

        auto match_results = match_template(formal.type, *initialized_type);
        if (!match_results.has_value() || !merge_template_matches(deduced_templates, *match_results))
        {
            co_return std::nullopt;
        }

        if (actual_is_value)
        {
            auto value = actual.template get_as< parameter_value_instantiation >();
            value.type = *initialized_type;
            co_return value;
        }

        co_return make_type_instantiation(*initialized_type);
    };

    auto defaulted_parameter = [](argif const& formal) -> parameter_instantiation
    {
        if (formal.requires_static_value)
        {
            throw std::logic_error("Defaulted static-value parameters are not supported");
        }
        return make_type_instantiation(formal.type);
    };

    for (auto const& [name, argument] : preargs.named)
    {
        auto it = ensig.interface.named.find(name);
        if (it == ensig.interface.named.end())
        {
            co_return std::nullopt;
        }

        auto initialized = co_await initialize_parameter(it->second, argument);
        if (!initialized.has_value())
        {
            co_return std::nullopt;
        }
        result.named[name] = std::move(*initialized);
    }

    for (auto const& [name, formal] : ensig.interface.named)
    {
        if (result.named.contains(name))
        {
            continue;
        }
        if (!formal.is_defaulted)
        {
            co_return std::nullopt;
        }
        result.named[name] = defaulted_parameter(formal);
    }

    for (std::size_t i = 0; i < preargs.positional.size(); i++)
    {
        auto const& formal = positional_formal_for(ensig.interface, pack_index, i);
        auto initialized = co_await initialize_parameter(formal, preargs.positional.at(i));
        if (!initialized.has_value())
        {
            co_return std::nullopt;
        }
        result.positional.push_back(std::move(*initialized));
    }

    if (!pack_index.has_value())
    {
        for (std::size_t i = preargs.positional.size(); i < ensig.interface.positional.size(); i++)
        {
            auto const& formal = ensig.interface.positional.at(i);
            if (!formal.is_defaulted)
            {
                co_return std::nullopt;
            }
            result.positional.push_back(defaulted_parameter(formal));
        }
    }

    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        co_yield rpnx::querygraph::debug_message("Ensig init with {} with {} yields {}", to_string(input.ensig.interface), to_string(input.params), to_string(result));
    }

    co_return result;
}
