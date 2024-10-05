// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/compiler.hpp>
#include <quxlang/res/functanoid_parameter_map.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_parameter_map)
{
    instanciation_reference func_ast = input;

    type_symbol selection = input.callee;

    assert(typeis< selection_reference >(selection));

    if (!typeis< selection_reference >(selection))
    {
        auto selection_opt = co_await *c->lk_functum_select_function(input);
        if (!selection_opt.has_value())
        {
            throw std::logic_error("No selection reference for functum.");
        }

        selection = selection_opt.value();
    }

    type_symbol func_name = input.callee;
    auto functum_instanciation_parameters = input.parameters;

    output_type result;

    selection_reference selection_sl = as< selection_reference >(selection);

    // TODO: support named parameters?
    for (std::size_t i = 0; i < functum_instanciation_parameters.positional_parameters.size(); i++)
    {
        auto template_arg_contextual = selection_sl.overload.call_parameters.positional_parameters.at(i);
        // TODO: should the selection reference be decontextualized early?

        auto template_arg = co_await *c->lk_canonical_symbol_from_contextual_symbol(template_arg_contextual, func_name);
        std::string template_arg_str = to_string(template_arg);

        type_symbol instanciation_arg = functum_instanciation_parameters.positional_parameters.at(i);
        assert(!is_contextual(instanciation_arg));
        std::string instanciation_arg_str = to_string(instanciation_arg);

        auto match_results = match_template(template_arg, instanciation_arg);
        assert(match_results.has_value());

        for (auto x : match_results.value().matches)
        {
            if (result.parameter_map.find(x.first) != result.parameter_map.end())
            {
                throw std::logic_error("Duplicate template parameter " + x.first + " redeclared in the same template instanciation.");
            }
            result.parameter_map[x.first] = x.second;
        }
    }

    co_return result;
}