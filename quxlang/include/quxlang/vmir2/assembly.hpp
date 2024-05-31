//
// Created by Ryan Nicholl on 5/30/24.
//

#ifndef RPNX_QUXLANG_ASSEMBLY_HEADER
#define RPNX_QUXLANG_ASSEMBLY_HEADER

#include <quxlang/vmir2/vmir2.hpp>
#include <string>

namespace quxlang::vmir2
{
    class assembler
    {
      public:
        std::string to_string(vm_instruction inst);

      private:

        std::string to_string_internal(vm_invoke inst)
    };
}; // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_ASSEMBLY_HEADER
