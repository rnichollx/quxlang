//
// Created by Ryan Nicholl on 11/5/23.
//
#include "../../../rpnx/include/rpnx/debug.hpp"
#include "quxlang/manipulators/argmanip.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/variant_utils.hpp"

#include "quxlang/res/function_declaration.hpp"

using namespace quxlang;

#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_declaration)
{
    selection_reference const& func_addr = input;

    std::string dbg_func_name = to_string(input);

    assert(!qualified_is_contextual(func_addr));


    auto entity_ast_v = co_await QUX_CO_DEP(symboid, (func_addr.templexoid));

    if (!typeis< functum >(entity_ast_v))
    {
        co_return std::nullopt;
        // throw std::logic_error("Getting function AST for non-functum entity");
    }

    functum const& functum_entity_ast_v = as< functum >(entity_ast_v);



    for (auto& func : functum_entity_ast_v.functions)
    {
        function_overload func_ol;
        func_ol.priority = func.header.priority;

        // TODO: reimplement this and the one in list_user_overloads as "decontextualize_function_overload"

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

        std::string s1 = to_string(func_ol.call_parameters);
        std::string s2 = to_string(func_addr.overload.call_parameters);


        std::string s3 = func_ol.builtin ? "builtin" : "not builtin";
        std::string s4 = func_addr.overload.builtin ? "builtin" : "not builtin";

        function_overload const & other_ol = func_addr.overload;
        if (func_ol == other_ol)
        {
            // TODO: this should check for multiple/duplicate matches.
            //  I wont do this now, it can be finished later on.
            QUX_CO_ANSWER(func);
        }
    }

    throw std::logic_error("no matching function overload");
}
