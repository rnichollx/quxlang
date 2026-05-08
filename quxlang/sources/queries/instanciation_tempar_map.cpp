// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/instanciation_tempar_map_spec.hpp>

#include "quxlang/data/temploid_instanciation_parameter_set.hpp"
#include "quxlang/manipulators/typeutils.hpp"

namespace
{
    /// Returns the index of the selected interface's positional pack, if present.
    auto positional_pack_index(quxlang::intertype const& interface) -> std::optional< std::size_t >
    {
        std::optional< std::size_t > result;
        for (std::size_t i = 0; i < interface.positional.size(); i++)
        {
            if (!interface.positional.at(i).is_pack)
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

    /// Returns the formal parameter for one concrete expanded positional argument.
    auto positional_formal_for(quxlang::intertype const& interface, std::optional< std::size_t > pack_index, std::size_t argument_index) -> quxlang::argif const&
    {
        if (pack_index.has_value() && argument_index >= *pack_index)
        {
            return interface.positional.at(*pack_index);
        }
        return interface.positional.at(argument_index);
    }

    /// Merges template parameter deductions while enforcing repeated named tempar consistency.
    auto merge_template_matches(quxlang::temploid_instanciation_parameter_set& result, quxlang::template_match_results const& matches) -> void
    {
        for (auto const& x : matches.matches)
        {
            auto existing = result.parameter_map.find(x.first);
            if (existing != result.parameter_map.end())
            {
                if (existing->second != x.second)
                {
                    throw quxlang::semantic_compilation_error("Template parameter " + x.first + " has inconsistent argument types in the same template instanciation.");
                }
                continue;
            }
            result.parameter_map[x.first] = x.second;
        }
    }
} // namespace


rpnx::querygraph::coroutine< quxlang::instanciation_tempar_map_spec > quxlang::instanciation_tempar_map_impl(instanciation_reference input)
{
    temploid_reference func_name = input.temploid;

    instanciation_tempar_map_query::output_type result;

    auto const pack_index = positional_pack_index(func_name.which.interface);
    auto const fixed_positional_count = pack_index.value_or(func_name.which.interface.positional.size());
    if ((!pack_index.has_value() && input.params.positional.size() != func_name.which.interface.positional.size()) || (pack_index.has_value() && input.params.positional.size() < fixed_positional_count))
    {
        throw quxlang::semantic_compilation_error("Instantiated function positional parameter count does not match the selected interface.");
    }


    // Positional parameters
    for (std::size_t i = 0; i < input.params.positional.size(); i++)
    {
        auto template_arg = positional_formal_for(func_name.which.interface, pack_index, i);
        // TODO: should the selection reference be decontextualized early?
        // TODO: Handle defaulted parameter logic if needed?

        type_symbol instanciation_arg = parameter_instantiation_type(input.params.positional.at(i));
        assert(!is_contextual(instanciation_arg));
        std::string instanciation_arg_str = to_string(instanciation_arg);

        auto match_results = match_template(template_arg.type, instanciation_arg);
        assert(match_results.has_value());
        merge_template_matches(result, match_results.value());
    }

    // Named parameters
    for (auto const& [name, arg_val] : input.params.named)
    {
        auto it = func_name.which.interface.named.find(name);
        if (it == func_name.which.interface.named.end())
        {
            throw quxlang::semantic_compilation_error("Unknown named parameter '" + name + "' for instanciation.");
        }
        auto const& template_arg = it->second;
        type_symbol instanciation_arg = parameter_instantiation_type(arg_val);
        assert(!is_contextual(instanciation_arg));
        auto match_results = match_template(template_arg.type, instanciation_arg);
        assert(match_results.has_value());
        merge_template_matches(result, *match_results);
    }

    co_return result;
}
