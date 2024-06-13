//
// Created by Ryan Nicholl on 6/12/24.
//


#include <quxlang/res/template_instanciation.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(template_instanciation)
{
   throw rpnx::unimplemented();
   co_return instanciation_reference{};
}