//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_PROCEDURE_INTERFACE_HEADER
#define RPNX_RYANSCRIPT1031_VM_PROCEDURE_INTERFACE_HEADER

#include "vm_type.hpp"
#include <vector>
#include <optional>
namespace rylang
{
   struct vm_procedure_interface
   {
       std::vector<vm_type> argument_types;
       std::optional<vm_type> return_type;
   };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_INTERFACE_HEADER
