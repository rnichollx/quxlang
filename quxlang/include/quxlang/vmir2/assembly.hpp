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
        std::string to_string(vmir2::functanoid_routine inst);
        std::string to_string(vmir2::vm_instruction inst);
        std::string to_string(vmir2::vm_slot slt);

      private:

        std::string to_string_internal(vmir2::access_field inst);
        std::string to_string_internal(vmir2::invoke inst);
        std::string to_string_internal(vmir2::invocation_args inst);
    };

}; // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_ASSEMBLY_HEADER
