//
// Created by Ryan Nicholl on 4/6/24.
//

#include <quxlang/compiler.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_select_function)
{
    if (typeis< selection_reference >(input.callee))
    {
        // TODO: We should identify a real match and error if this isn't a valid selection.
        // E.g. if there are type aliases, we should return the "real" type here instead of the type alias.
        // There should also be a selection error when this selection doesn't exist.
        // e.g. ::myint ALIAS I32;
        // ::foo FUNCTION(%x I32) ...
        // Would result in the following selection:
        // calle=foo@[::myint] params=(...) -> foo@[I32]

        QUX_CO_ANSWER(as< selection_reference >(input.callee));
    }

    auto sym_kind = co_await QUX_CO_DEP(symbol_type, (input.callee));


    throw rpnx::unimplemented();
}
