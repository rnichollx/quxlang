//
// Created by Ryan Nicholl on 11/5/23.
//
#include "rylang/manipulators/argmanip.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/variant_utils.hpp"

using namespace rylang;

rpnx::resolver_coroutine<compiler, function_ast > rylang::function_ast_resolver::co_process(rylang::compiler* c, qualified_symbol_reference func_addr)
{
    std::optional< call_parameter_information > overload_set;

    assert(!qualified_is_contextual(func_addr));

    std::string typestr = boost::apply_visitor(qualified_symbol_stringifier(), func_addr);

    // TODO: We should only strip the overload set from functions, not templates


    if (typeis< functanoid_reference >(func_addr))
    {
        auto args = as< functanoid_reference >(func_addr).parameters;
        overload_set = call_parameter_information{std::vector< qualified_symbol_reference >(args.begin(), args.end())};
        func_addr = qualified_parent(func_addr).value();
    }

    auto entity_ast_v = co_await *c->lk_entity_ast_from_canonical_chain(func_addr);

    if (entity_ast_v.type() != entity_type::function_type)
    {
        throw std::runtime_error("Getting function AST for non-functum entity");
    }

    functum_entity_ast functum_entity_ast_v = entity_ast_v.get_as< functum_entity_ast >();

    if (!overload_set.has_value())
    {
        if (functum_entity_ast_v.m_function_overloads.size() == 1)
        {
            co_return functum_entity_ast_v.m_function_overloads[0];
        }
        else
        {
            throw std::runtime_error("Requested resolution of function from funcctum is ambiguous");
        }
    }

    auto overload_set_value = overload_set.value();

    for (function_ast const& func : functum_entity_ast_v.m_function_overloads)
    {
        call_parameter_information cos_args = co_await *c->lk_call_params_of_function_ast(func, func_addr);

        bool callable = co_await *c->lk_overload_set_is_callable_with(overload_set_value, cos_args);
        // TODO: For now, just grab the first one that matches, later error on avoid ambiguous overloads.

        if (callable)
        {
            co_return func;
        }
    }

    throw std::runtime_error("no matching function overload");
}
