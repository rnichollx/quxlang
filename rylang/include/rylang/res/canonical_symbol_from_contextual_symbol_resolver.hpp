//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANONICAL_SYMBOL_FROM_CONTEXTUAL_SYMBOL_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CANONICAL_SYMBOL_FROM_CONTEXTUAL_SYMBOL_RESOLVER_HEADER

// input: contextual_type_reference
// output: canonical_type_reference

#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/contextual_type_reference.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"

namespace rylang
{
    class canonical_symbol_from_contextual_symbol_resolver : public rpnx::resolver_base< compiler, qualified_symbol_reference >
    {
      public:
        using key_type = contextual_type_reference;
        canonical_symbol_from_contextual_symbol_resolver(contextual_type_reference ref)
        {
            m_ref = ref;
        }

        virtual void process(compiler* c) override;

      private:
        contextual_type_reference m_ref;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CANONICAL_SYMBOL_FROM_CONTEXTUAL_SYMBOL_RESOLVER_HEADER
