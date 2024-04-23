//
// Created by Ryan Nicholl on 11/18/23.
//
#include "quxlang/compiler.hpp"

#include "quxlang/res/list_user_functum_overloads_resolver.hpp"

#include "quxlang/operators.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_user_functum_overloads)
{
    std::string name = to_string(input_val);

    auto const& func_addr = input_val;

    std::vector< function_overload > result;

    QUX_CO_GETDEP(maybe_functum_ast, symboid, (input_val));

    if (!typeis< functum >(maybe_functum_ast))
    {
        // TODO: Redirect to constructor?
        co_return {};
    }

    auto const& functum_v = as< functum >(maybe_functum_ast);

    for (auto const& func : functum_v.functions)
    {
        function_overload func_ol;
        func_ol.priority = func.header.priority;

        for (auto& param : func.header.call_parameters)
        {
            // TODO: Ideally we would somehow have this be in a separate resolver, which could aggregate the results and ensure there are no duplicates.

            auto type_cannonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (contextual_type_reference{.context = qualified_parent(func_addr).value(), .type = param.type}));

            if (param.api_name)
            {
                func_ol.call_parameters.named_parameters[param.api_name.value()] = type_cannonical;
            }
            else
            {
                func_ol.call_parameters.positional_parameters.push_back(type_cannonical);
            }
        }

        result.push_back(std::move(func_ol));
    }

    QUX_CO_ANSWER(result);
}
