//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/list_user_functum_overloads_resolver.hpp"

#include "rylang/operators.hpp"

rpnx::resolver_coroutine< rylang::compiler, std::set< rylang::call_parameter_information > > rylang::list_user_functum_overloads_resolver::co_process(compiler* c, type_symbol functum)
{

    std::string name = to_string(functum);
    auto exists = co_await *c->lk_entity_canonical_chain_exists(functum);
    c->lk_entity_canonical_chain_exists(functum)->debug_recursive();
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
