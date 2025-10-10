// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_VM_PROCEDURE2_HEADER_GUARD
#define QUXLANG_RES_VM_PROCEDURE2_HEADER_GUARD

#include "quxlang/data/vm_procedure.hpp"
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/res/resolver.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang
{


    QUX_CO_RESOLVER(vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);
    QUX_CO_RESOLVER(user_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);
    QUX_CO_RESOLVER(builtin_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);

    QUX_CO_RESOLVER(builtin_default_ctor_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);
    QUX_CO_RESOLVER(builtin_dtor_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);

    QUX_CO_RESOLVER(builtin_copy_ctor_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);
    QUX_CO_RESOLVER(builtin_move_ctor_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);
    QUX_CO_RESOLVER(builtin_swap_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);
    QUX_CO_RESOLVER(builtin_assignment_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);

    QUX_CO_RESOLVER(builtin_other_vm_procedure3, instanciation_reference, vmir2::functanoid_routine3);

}

#endif // RPNX_QUXLANG_VM_PROCEDURE2_HEADER
