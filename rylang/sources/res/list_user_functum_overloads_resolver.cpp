//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/list_user_functum_overloads_resolver.hpp"

#include "rylang/operators.hpp"

rpnx::resolver_coroutine< rylang::compiler, std::set< rylang::call_parameter_information > > rylang::list_user_functum_overloads_resolver::co_process(compiler* c, type_symbol functum)
{

    auto exists = co_await *c->lk_entity_canonical_chain_exists(functum);

    if (!exists)
    {
        co_return {};
    }

    std::set< call_parameter_information > result;

    auto functum_ast = co_await *c->lk_entity_ast_from_canonical_chain(functum);

    if (!std::holds_alternative< functum_entity_ast >(functum_ast.m_specialization.get()))
    {
        // TODO: Redirect to constructor?
        co_return {};
    }

    functum_entity_ast const& functum_e = std::get< functum_entity_ast >(functum_ast.m_specialization.get());

    for (function_ast const& f : functum_e.m_function_overloads)
    {
        result.insert(co_await *c->lk_call_params_of_function_ast(f, functum));
    }

    co_return result;
}
