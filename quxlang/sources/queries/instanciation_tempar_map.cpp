// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/instanciation_tempar_map_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::instanciation_tempar_map_spec > quxlang::instanciation_tempar_map_impl(instanciation_reference input)
{
    temploid_reference func_name = input.temploid;

    instanciation_tempar_map_query::output_type result;


    // Positional parameters
    for (std::size_t i = 0; i < input.params.positional.size(); i++)
    {
        auto template_arg = func_name.which.interface.positional.at(i);
        // TODO: should the selection reference be decontextualized early?
        // TODO: Handle defaulted parameter logic if needed?

        type_symbol instanciation_arg = input.params.positional.at(i);
        assert(!is_contextual(instanciation_arg));
        std::string instanciation_arg_str = to_string(instanciation_arg);

        auto match_results = match_template(template_arg.type, instanciation_arg);
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

    // Named parameters
    for (auto const& [name, arg_val] : input.params.named)
    {
        auto it = func_name.which.interface.named.find(name);
        if (it == func_name.which.interface.named.end())
        {
            throw std::logic_error("Unknown named parameter '" + name + "' for instanciation.");
        }
        auto const& template_arg = it->second;
        type_symbol instanciation_arg = arg_val;
        assert(!is_contextual(instanciation_arg));
        auto match_results = match_template(template_arg.type, instanciation_arg);
        assert(match_results.has_value());
        for (auto const& x : match_results->matches)
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
