// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_select_function_spec.hpp>

#include "quxlang/data/compilation_result.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>

#include <quxlang/macros.hpp>

namespace
{
    struct function_overload_candidate
    {
        std::uint64_t overload_id;
        quxlang::temploid_ensig ensig;
        std::optional< quxlang::instatype > initialized_params;
    };

    /// Returns the index of a selected interface's positional pack, if it has one.
    auto positional_pack_index(quxlang::temploid_ensig const& ensig) -> std::optional< std::size_t >
    {
        std::optional< std::size_t > result;
        for (std::size_t i = 0; i < ensig.interface.positional.size(); i++)
        {
            if (!ensig.interface.positional.at(i).is_pack)
            {
                if (result.has_value())
                {
                    throw quxlang::semantic_compilation_error("A positional parameter cannot follow a positional variadic pack");
                }
                continue;
            }
            if (result.has_value())
            {
                throw quxlang::semantic_compilation_error("Only one positional variadic pack is supported");
            }
            result = i;
        }
        return result;
    }

    /// Returns true when the selected interface has a positional pack.
    auto has_positional_pack(quxlang::temploid_ensig const& ensig) -> bool
    {
        return positional_pack_index(ensig).has_value();
    }

    /// Returns the number of ordinary positional parameters before a pack, or the full positional count.
    auto fixed_positional_prefix_count(quxlang::temploid_ensig const& ensig) -> std::size_t
    {
        return positional_pack_index(ensig).value_or(ensig.interface.positional.size());
    }

    /// Returns the formal parameter used for a concrete expanded positional argument.
    auto positional_formal_for(quxlang::temploid_ensig const& ensig, std::size_t argument_index) -> quxlang::argif const&
    {
        auto const pack_index = positional_pack_index(ensig);
        if (pack_index.has_value() && argument_index >= *pack_index)
        {
            return ensig.interface.positional.at(*pack_index);
        }
        return ensig.interface.positional.at(argument_index);
    }
} // namespace


rpnx::querygraph::coroutine< quxlang::functum_select_function_spec > quxlang::functum_select_function_impl(initialization_reference input)
{
    if (typeis< temploid_reference >(input.initializee))
    {
        auto const& selected = as< temploid_reference >(input.initializee);
        auto const& selected_kind = co_await rpnx::querygraph::request< symbol_type_query >(selected);

        if (selected_kind == symbol_kind::template_)
        {
            throw quxlang::compiler_bug("functum_select_function received a template selection. Function templates are not directly callable; set the template arguments manually before resolving a callable function.");
        }

        if (selected_kind != symbol_kind::function)
        {
            throw quxlang::compiler_bug("functum_select_function received a temploid selection that is not a function.");
        }

        // TODO: We should identify a real match and error if this isn't a valid selection.
        // E.g. if there are type aliases, we should return the "real" type here instead of the type alias.
        // There should also be a selection error when this selection doesn't exist.
        // e.g. ::myint ALIAS I32;
        // ::foo FUNCTION(%x I32) ...
        // Would result in the following selection:
        // calle=foo#[::myint] params=(...) -> foo#[I32]

        co_return selected;
    }

    auto sym_kind = co_await rpnx::querygraph::request< symbol_type_query >(input.initializee);

    if (sym_kind != symbol_kind::functum)
    {
        co_return std::nullopt;
    }

    std::vector< function_overload_candidate > overloads;
    auto const& user_overloads = co_await rpnx::querygraph::request< functum_map_user_formal_ensigs_query >(input.initializee);
    auto const& builtin_overloads = co_await rpnx::querygraph::request< functum_builtin_overloads_query >(input.initializee);

    overloads.reserve(user_overloads.size() + builtin_overloads.size());
    for (std::size_t user_index = 0; user_index < user_overloads.size(); user_index++)
    {
        bool found = false;
        for (auto const& [ensig, index] : user_overloads)
        {
            if (index == user_index)
            {
                overloads.push_back(function_overload_candidate{
                    .overload_id = static_cast< std::uint64_t >(user_index),
                    .ensig = ensig,
                });
                found = true;
                break;
            }
        }

        if (!found)
        {
            throw compiler_bug("User functum overload IDs are not contiguous");
        }
    }

    std::uint64_t builtin_index = static_cast< std::uint64_t >(user_overloads.size());
    for (auto const& ensig : builtin_overloads)
    {
        overloads.push_back(function_overload_candidate{
            .overload_id = builtin_index,
            .ensig = ensig,
        });
        builtin_index++;
    }

    auto describe_overload = [&](function_overload_candidate const& candidate) -> std::string
    {
        return quxlang::to_string(temploid_reference{
            .templexoid = input.initializee,
            .overload_id = candidate.overload_id,
        }) + " " + quxlang::to_string(candidate.ensig.interface);
    };

    std::vector< function_overload_candidate > best_match;
    std::optional< std::int64_t > highest_priority;

    std::string context_type = "";
    switch (input.adaptations)
    {
    case allowed_adaptations::none:
        context_type = "exact";
        break;
    case allowed_adaptations::source_rebinding:
        context_type = "source_rebinding";
        break;
    case allowed_adaptations::class_conversions:
        context_type = "class_conversions";
        break;
    case allowed_adaptations::destination_rebinding:
        context_type = "destination_rebinding";
        break;
    }

    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        co_yield rpnx::querygraph::debug_message("Select for {} in {}", quxlang::to_string(input), context_type);
    }

    type_symbol selection_type_of_this = void_type{};
    if (typeis< submember >(input.initializee))
    {
        selection_type_of_this = as< submember >(input.initializee).of;
    }

    auto ranked_this_param_type = [&](auto&& self, type_symbol type) -> type_symbol
    {
        if (typeis< thistype >(type) && !typeis< void_type >(selection_type_of_this))
        {
            return selection_type_of_this;
        }
        if (typeis< ptrref_type >(type))
        {
            ptrref_type output = as< ptrref_type >(type);
            output.target = self(self, output.target);
            return output;
        }
        if (typeis< nvalue_slot >(type))
        {
            nvalue_slot output = as< nvalue_slot >(type);
            output.target = self(self, output.target);
            return output;
        }
        if (typeis< dvalue_slot >(type))
        {
            dvalue_slot output = as< dvalue_slot >(type);
            output.target = self(self, output.target);
            return output;
        }
        if (typeis< attached_type_reference >(type))
        {
            attached_type_reference output = as< attached_type_reference >(type);
            output.carrying_type = self(self, output.carrying_type);
            output.attached_symbol = self(self, output.attached_symbol);
            return output;
        }
        return type;
    };

    for (auto const& o : overloads)
    {
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("  Found overload: {}", describe_overload(o));
        }
    }

    for (auto const& o : overloads)
    {
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::stringstream ss;
            ss << "Considering overload " << describe_overload(o) << " with parameters " << quxlang::to_string(input.parameters);

            if (ss.str() == "Considering overload BYTE::.OPERATOR==#[0] [@OTHER BYTE, @THIS BYTE] with parameters CALLABLE(@OTHER NUMERIC_LITERAL, @THIS & BYTE)")
            {
                int breakpoint = 0;
            }


            co_yield rpnx::querygraph::debug_message("  {}", ss.str());
        }

        std::optional< instatype > candidate = co_await rpnx::querygraph::request< function_ensig_init_with_query >({
            .ensig = o.ensig,
            .params = input.parameters,
            .adaptations = input.adaptations,
            .type_of_this = selection_type_of_this,
        });

        if (candidate && typeis< submember >(input.initializee))
        {
            auto const& member = as< submember >(input.initializee);
            if (member.name == "CONSTRUCTOR" && candidate->named.contains("OTHER"))
            {
                auto const& other_type = parameter_instantiation_type(candidate->named.at("OTHER"));
                if (!is_ref(other_type) && other_type == member.of)
                {
                    candidate.reset();
                }
            }
        }

        // Evaluate ENABLE_IF in the proper context after instantiation
        if (candidate && o.ensig.enable_if)
        {
            constexpr_input cx_input;
            cx_input.expr = *o.ensig.enable_if;
            temploid_reference tr{
                .templexoid = input.initializee,
                .overload_id = o.overload_id,
            };
            instanciation_reference inst{.temploid = tr, .params = *candidate};
            // Use the instantiated function context so pack type inspection can see the expanded parameters.
            cx_input.context = inst;
            bool include = co_await rpnx::querygraph::request< constexpr_bool_query >(cx_input);
            if (!include)
            {
                candidate.reset();
            }
        }

        if (candidate)
        {
            function_overload_candidate accepted = o;
            accepted.initialized_params = *candidate;
            std::size_t priority = o.ensig.priority.value_or(0);

            if (!highest_priority || priority > *highest_priority)
            {
                highest_priority = priority;
                best_match.clear();
                best_match.push_back(std::move(accepted));
            }
            else if (priority == *highest_priority)
            {
                best_match.push_back(std::move(accepted));
            }
        }
    }

    if (best_match.size() == 0)
    {
        QUX_WHY("No matching overloads");
        co_return std::nullopt;
        // throw quxlang::semantic_compilation_error("No matching overloads");
    }
    else if (best_match.size() > 1)
    {
        std::vector< function_overload_candidate > undominated;

        for (std::size_t candidate_index = 0; candidate_index < best_match.size(); ++candidate_index)
        {
            function_overload_candidate const& candidate = best_match.at(candidate_index);
            bool dominated = false;

            for (std::size_t other_index = 0; other_index < best_match.size(); ++other_index)
            {
                if (candidate_index == other_index)
                {
                    continue;
                }

                function_overload_candidate const& other = best_match.at(other_index);
                bool other_better = false;
                bool candidate_better = false;

                for (auto const& [name, arg] : input.parameters.named)
                {
                    auto const& arg_type = parameter_instantiation_type(arg);
                    type_symbol candidate_param = ranked_this_param_type(ranked_this_param_type, candidate.ensig.interface.named.at(name).type);
                    type_symbol other_param = ranked_this_param_type(ranked_this_param_type, other.ensig.interface.named.at(name).type);

                    auto other_beats_candidate = co_await rpnx::querygraph::request< argument_adaptation_is_better_fit_query >(argument_adaptation_better_fit_input{
                        .from = arg_type,
                        .better_to = other_param,
                        .worse_to = candidate_param,
                        .adaptations = input.adaptations,
                    });

                    auto candidate_beats_other = co_await rpnx::querygraph::request< argument_adaptation_is_better_fit_query >(argument_adaptation_better_fit_input{
                        .from = arg_type,
                        .better_to = candidate_param,
                        .worse_to = other_param,
                        .adaptations = input.adaptations,
                    });

                    other_better = other_better || other_beats_candidate;
                    candidate_better = candidate_better || candidate_beats_other;
                }

                for (std::size_t i = 0; i < input.parameters.positional.size(); i++)
                {
                    auto const& arg_type = parameter_instantiation_type(input.parameters.positional.at(i));
                    type_symbol candidate_param = ranked_this_param_type(ranked_this_param_type, positional_formal_for(candidate.ensig, i).type);
                    type_symbol other_param = ranked_this_param_type(ranked_this_param_type, positional_formal_for(other.ensig, i).type);

                    auto other_beats_candidate = co_await rpnx::querygraph::request< argument_adaptation_is_better_fit_query >(argument_adaptation_better_fit_input{
                        .from = arg_type,
                        .better_to = other_param,
                        .worse_to = candidate_param,
                        .adaptations = input.adaptations,
                    });

                    auto candidate_beats_other = co_await rpnx::querygraph::request< argument_adaptation_is_better_fit_query >(argument_adaptation_better_fit_input{
                        .from = arg_type,
                        .better_to = candidate_param,
                        .worse_to = other_param,
                        .adaptations = input.adaptations,
                    });

                    other_better = other_better || other_beats_candidate;
                    candidate_better = candidate_better || candidate_beats_other;
                }

                if (other_better && !candidate_better)
                {
                    dominated = true;
                    break;
                }
            }

            if (!dominated)
            {
                undominated.push_back(candidate);
            }
        }

        best_match = std::move(undominated);
    }

    if (best_match.size() > 1)
    {
        std::vector< function_overload_candidate > non_variadic;
        for (auto const& item : best_match)
        {
            if (!has_positional_pack(item.ensig))
            {
                non_variadic.push_back(item);
            }
        }
        if (!non_variadic.empty())
        {
            best_match = std::move(non_variadic);
        }
    }

    if (best_match.size() > 1)
    {
        std::size_t best_fixed_prefix = 0;
        bool saw_variadic = false;
        for (auto const& item : best_match)
        {
            if (!has_positional_pack(item.ensig))
            {
                continue;
            }
            saw_variadic = true;
            best_fixed_prefix = std::max(best_fixed_prefix, fixed_positional_prefix_count(item.ensig));
        }

        if (saw_variadic)
        {
            std::vector< function_overload_candidate > largest_fixed_prefix;
            for (auto const& item : best_match)
            {
                if (has_positional_pack(item.ensig) && fixed_positional_prefix_count(item.ensig) == best_fixed_prefix)
                {
                    largest_fixed_prefix.push_back(item);
                }
            }
            best_match = std::move(largest_fixed_prefix);
        }
    }

    if (best_match.size() > 1)
    {
        std::stringstream message;
        message << "Ambiguous overload resolution for " << to_string(input) << " candidates:";
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message(" Ambiguous overloads for {}:", to_string(input));
        }
        for (auto const& item : best_match)
        {
            message << "\n  " << describe_overload(item);
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("   Ambiguous candidate: {}", describe_overload(item));
            }
        }
        throw semantic_compilation_error(message.str());
    }
    auto best_ref = temploid_reference{
        .templexoid = input.initializee,
        .overload_id = best_match.front().overload_id,
    };
    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        co_yield rpnx::querygraph::debug_message(" Best match for {} is {}", to_string(input), describe_overload(best_match.front()));
    }

    co_return best_ref;
}
