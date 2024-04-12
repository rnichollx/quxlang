//
// Created by Ryan Nicholl on 11/18/23.
//
#include "quxlang/compiler.hpp"

#include "quxlang/res/list_user_functum_overloads_resolver.hpp"

#include "quxlang/operators.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_user_functum_overloads)
{
    auto functum_v = input_val;
    std::string name = to_string(functum_v);
    QUX_CO_GETDEP(exists, entity_canonical_chain_exists, (functum_v));
    if (!exists)
    {
        co_return {};
    }

    std::set< ast2_function_header > result;

    QUX_CO_GETDEP(maybe_functum_ast, entity_ast_from_canonical_chain, (functum_v));

    if (!typeis< functum >(maybe_functum_ast))
    {
        // TODO: Redirect to constructor?
        co_return {};
    }

    functum const& functum_ast = as< functum >(maybe_functum_ast);

    for (auto const& f : functum_ast.functions)
    {
        result.insert(f.first);
    }

    QUX_CO_ANSWER(result);
}
