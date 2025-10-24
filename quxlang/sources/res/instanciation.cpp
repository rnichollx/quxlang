// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler.hpp>
#include <quxlang/res/instanciation.hpp>


#include "quxlang/res/function.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(instanciation_tempar_map)
{
    temploid_reference func_name = input.temploid;

    output_type result;


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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_instanciation)
{
    if (!typeis< temploid_reference >(input_val.initializee))
    {
        throw std::logic_error("Internal Compiler Error(this is a compiler bug): Cannot instanciate a non-function with 'function_instanciation' resolver");
    }

    temploid_reference const& sel_ref = as< temploid_reference >(input_val.initializee);


    // Get the overload?
    auto call_set = co_await QUX_CO_DEP(function_ensig_init_with, (ensig_initialization{.ensig = sel_ref.which, .params = input_val.parameters}));

    if (!call_set)
    {
        QUX_WHY("No overload set found for function instanciation");
        QUX_CO_ANSWER(std::nullopt);
    }

    auto result = instanciation_reference{.temploid = sel_ref, .params=call_set.value()};
    QUX_CO_ANSWER(result);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(instanciation)
{
    if (input.init_kind == parameter_init_kind::none)
    {
        auto callers =  this->dependents();

        for (auto const& call : callers)
        {
            std::cout << "Caller: " << call->question() << std::endl;
        }

        throw std::logic_error("invalid construction");
    }
    std::string dbg_input = to_string(input_val);
    type_symbol templexoid_symbol = input_val.initializee;

    auto kind = co_await QUX_CO_DEP(symbol_type, (templexoid_symbol));

    if (kind == symbol_kind::functum)
    {
        co_return co_await QUX_CO_DEP(functum_initialize, (input_val));
    }
    else if (kind == symbol_kind::function)
    {
        co_return co_await QUX_CO_DEP(function_instanciation, (input_val));
    }
    else if (kind == symbol_kind::template_)
    {
        co_return co_await QUX_CO_DEP(template_instanciation, (input_val));
    }
    else if (kind == symbol_kind::templex)
    {
       throw rpnx::unimplemented();
       // co_return co_await QUX_CO_DEP(templex_instanciation, (input_val));
    }
    else
    {
        co_return std::nullopt;
      //  throw std::logic_error("Cannot instanciate non-templexoid");
    }
}