//
// Created by Ryan Nicholl on 11/18/23.
//
#include "quxlang/compiler.hpp"

#include "quxlang/res/list_user_functum_overloads_resolver.hpp"

#include "quxlang/operators.hpp"


QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_user_functum_overloads)
{
    auto functum = input_val;
    std::string name = to_string(functum);
    auto exists = co_await *c->lk_entity_canonical_chain_exists(functum);
    //c->lk_entity_canonical_chain_exists(functum)->debug_recursive();
    if (!exists)
    {
        co_return {};
    }

    std::set< call_parameter_information > result;

    auto maybe_functum_ast = co_await *c->lk_entity_ast_from_canonical_chain(functum);

    if (!typeis<ast2_functum>(maybe_functum_ast))
    {
        // TODO: Redirect to constructor?
        co_return {};
    }

    ast2_functum const& functum_ast = as<ast2_functum>(maybe_functum_ast);

    for (ast2_function_declaration const& f : functum_ast.functions)
    {
        result.insert(co_await *c->lk_call_params_of_function_ast(f, functum));
    }

    co_return result;
}
