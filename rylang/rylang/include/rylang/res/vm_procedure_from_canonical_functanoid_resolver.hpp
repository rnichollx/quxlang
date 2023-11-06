//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/data/vm_procedure.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    class vm_procedure_from_canonical_functanoid_resolver : public rpnx::resolver_base< compiler, vm_procedure >
    {
      public:
        using key_type = qualified_symbol_reference;

        vm_procedure_from_canonical_functanoid_resolver(qualified_symbol_reference func_addr)
        : m_func_name(func_addr)
        {
        }

        void process(compiler* c);

      private:
        qualified_symbol_reference m_func_name;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
