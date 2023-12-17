//
// Created by Ryan Nicholl on 11/5/23.
//
#include "rylang/debug.hpp"
#include "rylang/manipulators/argmanip.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/variant_utils.hpp"

using namespace rylang;

rpnx::resolver_coroutine< compiler, ast2_function_declaration > rylang::function_ast_resolver::co_process(rylang::compiler* c, type_symbol func_addr)
{
    std::optional< call_parameter_information > overload_set;

    assert(!qualified_is_contextual(func_addr));

    std::string typestr = to_string(func_addr);

    // TODO: We should only strip the overload set from functions, not templates

    if (typeis< instanciation_reference >(func_addr))
    {
        auto args = as< instanciation_reference >(func_addr).parameters;
        overload_set = call_parameter_information{std::vector< type_symbol >(args.begin(), args.end())};
        func_addr = qualified_parent(func_addr).value();
    }

    auto entity_ast_v = co_await *c->lk_entity_ast_from_canonical_chain(func_addr);

    if (!typeis<ast2_fuctum>(entity_ast_v))
    {
        throw std::runtime_error("Getting function AST for non-functum entity");
    }

    ast2_functum const & functum_entity_ast_v = as<ast2_functum>(entity_ast_v);

    if (!overload_set.has_value())
    {
        if (functum_entity_ast_v.functions.size() == 1)
        {
            co_return functum_entity_ast_v.functions[0];
        }
        else
        {
            throw std::runtime_error("Requested resolution of function from funcctum is ambiguous");
        }
    }

    auto overload_set_value = overload_set.value();
    RYLANG_DEBUG(std::string overload_set_str = to_string(overload_set_value));

    if (typestr == "[[module: main]]::quz::bif::box::bif(MUT& I32)")
    {
        std::string();
    }

    for (function_ast const& func : functum_entity_ast_v.m_function_overloads)
    {
        call_parameter_information cos_args = co_await *c->lk_call_params_of_function_ast(func, func_addr);

        RYLANG_DEBUG(std::cout << debug_recursive());

        bool callable = co_await *c->lk_overload_set_is_callable_with(cos_args, overload_set_value);
        // TODO: For now, just grab the first one that matches, later error on avoid ambiguous overloads.

        if (typestr == "[[module: main]]::quz::bif::box::bif(MUT& I32)")
        {
            std::string();
        }

        if (callable)
        {
            co_return func;
        }
    }

    throw std::runtime_error("no matching function overload");
}
