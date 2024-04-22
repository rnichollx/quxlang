//
// Created by Ryan Nicholl on 4/6/24.
//

#include <quxlang/compiler.hpp>
#include <quxlang/res/functum_selection.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_selection)
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


        QUX_CO_ANSWER(as<selection_reference>(input.callee));
    }





    QUX_CO_GETDEP(sym, symboid, (input.callee));

    if (!typeis< functum >(ast))
    {
        throw std::logic_error("Cannot call non-functum");
    }

    auto const& func = as< functum >(ast);

    std::set<function_overload> eligible_overloads;

    for (auto const& overload : func.functions) {
       function_overload func_header = overload.first;
       auto func_args = func_header.call_parameters;

       // TODO: Finish this code

    }

    rpnx::unimplemented();
}
