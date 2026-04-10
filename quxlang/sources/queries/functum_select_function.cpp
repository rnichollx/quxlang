// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_select_function_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>


rpnx::querygraph::coroutine< quxlang::functum_select_function_spec > quxlang::functum_select_function_impl(initialization_reference input)
{

    auto input_str = to_string(input);

    auto input_functum_str = quxlang::to_string(input.initializee);

    if (typeis< temploid_reference >(input.initializee))
    {
        auto const& selected = as< temploid_reference >(input.initializee);
        auto selected_kind = co_await rpnx::querygraph::request< symbol_type_query >(selected);

        if (selected_kind == symbol_kind::template_)
        {
            throw std::logic_error("functum_select_function received a template selection. Function templates are not directly callable; set the template arguments manually before resolving a callable function.");
        }

        if (selected_kind != symbol_kind::function)
        {
            throw std::logic_error("functum_select_function received a temploid selection that is not a function.");
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

    auto overloads = co_await rpnx::querygraph::request< functum_overloads_query >(input.initializee);

    std::vector< temploid_ensig > best_match;
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

    std::cout << "Select for " << quxlang::to_string(input) << " in " << context_type << std::endl;

    for (auto const& o : overloads)
    {
        std::cout << "  Found overload: " << quxlang::to_string(temploid_reference{.templexoid = input.initializee, .which = o}) << std::endl;
    }

    for (auto const& o : overloads)
    {
        std::stringstream ss;
        ss << "Considering overload " << quxlang::to_string(temploid_reference{.templexoid = input.initializee, .which = o}) << " with parameters " << quxlang::to_string(input.parameters);

        if (ss.str() == "Considering overload BYTE::.OPERATOR==#[@OTHER BYTE, @THIS BYTE] with parameters CALLABLE(@OTHER NUMERIC_LITERAL, @THIS & BYTE)")
        {
            int breakpoint = 0;
        }

        std::cout << "  " << ss.str() << std::endl;

        std::optional< invotype > candidate = co_await rpnx::querygraph::request< function_ensig_init_with_query >({.ensig = o, .params = input.parameters, .adaptations = input.adaptations});

        if (candidate && typeis< submember >(input.initializee))
        {
            auto const& member = as< submember >(input.initializee);
            if (member.name == "CONSTRUCTOR" && candidate->named.contains("OTHER"))
            {
                auto const& other_type = candidate->named.at("OTHER");
                if (!is_ref(other_type) && other_type == member.of)
                {
                    candidate.reset();
                }
            }
        }

        // Evaluate ENABLE_IF in the proper context after instantiation
        if (candidate && o.enable_if)
        {
            constexpr_input cx_input;
            cx_input.expr = *o.enable_if;
            // Use the parent of the functum symbol as context when available
            cx_input.context = type_parent(input.initializee).value_or(context_reference{});
            // Load instantiated named parameters into constexpr scoped definitions
            for (auto const& [n, t] : candidate->named)
            {
                cx_input.scoped_definitions[n] = t;
            }
            // Also load ensig template parameters (tempars) mapped during instantiation
            {
                temploid_reference tr{.templexoid = input.initializee, .which = o};
                instanciation_reference inst{.temploid = tr, .params = *candidate};
                auto tempar_map = co_await rpnx::querygraph::request< instanciation_tempar_map_query >(inst);
                for (auto const& [name, t] : tempar_map.parameter_map)
                {
                    cx_input.scoped_definitions[name] = t;
                }
            }
            bool include = co_await rpnx::querygraph::request< constexpr_bool_query >(cx_input);
            if (!include)
            {
                candidate.reset();
            }
        }

        if (candidate)
        {
            std::size_t priority = o.priority.value_or(0);

            if (!highest_priority || priority > *highest_priority)
            {
                highest_priority = priority;
                best_match.clear();
                best_match.push_back(o);
            }
            else if (priority == *highest_priority)
            {
                best_match.push_back(o);
            }
        }
    }

    if (best_match.size() == 0)
    {
        QUX_WHY("No matching overloads");
        co_return std::nullopt;
        // throw std::logic_error("No matching overloads");
    }
    else if (best_match.size() > 1)
    {
        std::vector< temploid_ensig > undominated;

        for (auto const& candidate : best_match)
        {
            bool dominated = false;

            for (auto const& other : best_match)
            {
                if (candidate == other)
                {
                    continue;
                }

                bool other_better = false;
                bool candidate_better = false;

                for (auto const& [name, arg_type] : input.parameters.named)
                {
                    auto const& candidate_param = candidate.interface.named.at(name).type;
                    auto const& other_param = other.interface.named.at(name).type;

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
                    auto const& arg_type = input.parameters.positional.at(i);
                    auto const& candidate_param = candidate.interface.positional.at(i).type;
                    auto const& other_param = other.interface.positional.at(i).type;

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
        std::cout << " Ambiguous overloads for " << to_string(input) << ":" << std::endl;
        for (auto const& item : best_match)
        {
            std::cout << "   Ambiguous candidate: " << to_string(temploid_reference{.templexoid = input.initializee, .which = item}) << std::endl;
        }
        throw std::logic_error("Ambiguous overload resolution");
    }
    auto best_ref = temploid_reference{.templexoid = input.initializee, .which = best_match.front()};
    std::cout << " Best match for " << to_string(input) << " is " << to_string(best_ref) << std::endl;

    co_return best_ref;
}
