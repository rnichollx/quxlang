//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_PROCEDURE_INTERFACE_HEADER
#define RPNX_RYANSCRIPT1031_VM_PROCEDURE_INTERFACE_HEADER

#include "vm_type.hpp"
#include <vector>
#include <optional>

#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
   struct vm_procedure_interface
   {

       std::optional<qualified_symbol_reference> return_type;
       std::vector<qualified_symbol_reference> argument_types;
   };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_INTERFACE_HEADER
