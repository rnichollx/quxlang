//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD
#define RYLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD

#include "vm_type.hpp"
#include <vector>
#include <optional>

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
   struct vm_procedure_interface
   {

       std::optional<type_symbol> return_type;
       std::vector<type_symbol> argument_types;
   };
} // namespace rylang

#endif // RYLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD
