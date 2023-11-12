//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_GENERATION_FRAMEINFO_HEADER
#define RPNX_RYANSCRIPT1031_VM_GENERATION_FRAMEINFO_HEADER

#include "rylang/data/vm_frame_variable.hpp"

namespace rylang
{

   struct vm_generation_frame_info
   {
      std::vector<vm_frame_variable > variables;
      qualified_symbol_reference  context;
   };
}

#endif // RPNX_RYANSCRIPT1031_VM_GENERATION_FRAMEINFO_HEADER
