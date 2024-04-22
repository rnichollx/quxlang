//
// Created by Ryan Nicholl on 4/22/24.
//

#include <quxlang/compiler.hpp>
#include <quxlang/res/temploid_instanciation.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(templexoid_instanciation)
{

    type_symbol templexoid_symbol;
    if (typeis< selection_reference >(input_val.callee))
    {
        templexoid_symbol = as< selection_reference >(input_val.callee).callee;
    }
    else
    {
        templexoid_symbol = input_val.callee;
    }

    auto sym = co_await QUX_CO_DEP(symboid, (input_val.callee));

    if (typeis< functum >(sym))
    {
        auto sel = co_await QUX_CO_DEP(callee_temploid_instanciation, (sym));
    }
}