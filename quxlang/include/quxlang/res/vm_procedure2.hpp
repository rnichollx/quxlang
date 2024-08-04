//
// Created by Ryan Nicholl on 5/11/24.
//

#ifndef RPNX_QUXLANG_VM_PROCEDURE2_HEADER
#define RPNX_QUXLANG_VM_PROCEDURE2_HEADER

#include "quxlang/data/vm_procedure.hpp"
#include <quxlang/res/resolver.hpp>
#include <quxlang/vmir2/vmir2.hpp>
namespace quxlang
{
    QUX_CO_RESOLVER(vm_procedure2, type_symbol, vmir2::functanoid_routine2);
}

#endif // RPNX_QUXLANG_VM_PROCEDURE2_HEADER
