//
// Created by Ryan Nicholl on 3/2/24.
//
#include <quxlang/res/type_symbol_kind_resolver.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(type_symbol_kind)
{
    QUX_CO_GETDEP(ast, entity_ast_from_canonical_chain, (input_val));

    if (typeis< ast2_functum >(ast))
    {
        QUX_CO_ANSWER(symbol_kind::functum_kind);
    }

    QUX_CO_ANSWER(rpnx::unimplemented());
}