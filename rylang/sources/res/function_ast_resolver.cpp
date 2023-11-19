//
// Created by Ryan Nicholl on 11/5/23.
//
#include "rylang/compiler.hpp"
#include "rylang/manipulators/argmanip.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/variant_utils.hpp"

void rylang::function_ast_resolver::process(rylang::compiler* c)
{
    qualified_symbol_reference func_addr = m_function_name;
    std::optional< call_parameter_information > overload_set;


    assert(!qualified_is_contextual(func_addr));


    std::string typestr = boost::apply_visitor(qualified_symbol_stringifier(), func_addr);

    // TODO: We should only strip the overload set from functions, not templates

    qualified_symbol_reference qf = functanoid_reference{};
    assert(typeis< functanoid_reference >(qf  ));
    if (typeis< functanoid_reference >(func_addr))
    {
        auto args = as< functanoid_reference >(func_addr).parameters;
        overload_set = call_parameter_information{std::vector< qualified_symbol_reference >(args.begin(), args.end())};
        func_addr = qualified_parent(func_addr).value();
    }

    auto function_ast_dep = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(func_addr);
        });

    if (!ready())
    {
        return;
    }

    entity_ast entity_ast_v = function_ast_dep->get();

    if (entity_ast_v.type() != entity_type::function_type)
    {
        throw std::runtime_error("cannot generate vm code for non-function entity");
    }

    functum_entity_ast functum_entity_ast_v = entity_ast_v.get_as< functum_entity_ast >();

    if (!overload_set.has_value())
    {
        if (functum_entity_ast_v.m_function_overloads.size() == 1)
        {
            set_value(functum_entity_ast_v.m_function_overloads[0]);
        }
        else
        {
            throw std::runtime_error("requested vm_procedure is ambiguous, no overload set provided");
        }
    }

    auto overload_set_value = overload_set.value();

    for (function_ast const& func : functum_entity_ast_v.m_function_overloads)
    {
        call_parameter_information cos_args = to_call_overload_set(func.args);
        auto callable_dp = get_dependency(
            [&]
            {
                return c->lk_overload_set_is_callable_with(overload_set_value, cos_args);
            });

        if (!ready()) return;

        bool callable = callable_dp->get();
        // TODO: For now, just grab the first one that matches, later error on avoid ambiguous overloads.

        if (callable)
        {
            set_value(func);
            return;
        }
    }

    throw std::runtime_error("no matching function overload");
}
