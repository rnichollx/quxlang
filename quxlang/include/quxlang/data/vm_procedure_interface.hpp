//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD
#define QUXLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD

#include "vm_type.hpp"
#include <vector>
#include <optional>

#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
   struct vm_procedure_interface
   {

       std::optional<type_symbol> return_type;
       std::vector<type_symbol> argument_types;

       std::strong_ordering operator<=>(const vm_procedure_interface& other) const
       {
           return rpnx::compare(return_type, other.return_type, argument_types, other.argument_types);
       }
   };
} // namespace quxlang

#endif // QUXLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD
