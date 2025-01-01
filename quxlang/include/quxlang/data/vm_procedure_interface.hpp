// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_PROCEDURE_INTERFACE_HEADER_GUARD
#define QUXLANG_DATA_VM_PROCEDURE_INTERFACE_HEADER_GUARD

#include "vm_type.hpp"
#include <optional>
#include <vector>

#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    struct vm_procedure_interface
    {
        // TODO: replace with call_type
        std::optional< type_symbol > return_type;
        intertype argument_types;

        RPNX_MEMBER_METADATA(vm_procedure_interface, return_type, argument_types);
    };
} // namespace quxlang

#endif // QUXLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD
