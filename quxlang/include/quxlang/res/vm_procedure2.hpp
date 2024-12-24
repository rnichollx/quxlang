// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_VM_PROCEDURE2_HEADER_GUARD
#define QUXLANG_RES_VM_PROCEDURE2_HEADER_GUARD

#include "quxlang/data/vm_procedure.hpp"
#include <quxlang/res/resolver.hpp>
#include <quxlang/vmir2/vmir2.hpp>
namespace quxlang
{
    QUX_CO_RESOLVER(vm_procedure2, type_symbol, vmir2::functanoid_routine2);
    QUX_CO_RESOLVER(user_vm_procedure2, type_symbol, vmir2::functanoid_routine2);
    QUX_CO_RESOLVER(builtin_vm_procedure2, type_symbol, vmir2::functanoid_routine2);


    QUX_CO_RESOLVER(builtin_ctor_vm_procedure2, type_symbol, vmir2::functanoid_routine2);
    QUX_CO_RESOLVER(builtin_dtor_vm_procedure2, type_symbol, vmir2::functanoid_routine2);
}

#endif // RPNX_QUXLANG_VM_PROCEDURE2_HEADER
