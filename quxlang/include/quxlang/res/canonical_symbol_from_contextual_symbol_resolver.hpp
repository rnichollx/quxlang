//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef QUXLANG_CANONICAL_SYMBOL_FROM_CONTEXTUAL_SYMBOL_RESOLVER_HEADER_GUARD
#define QUXLANG_CANONICAL_SYMBOL_FROM_CONTEXTUAL_SYMBOL_RESOLVER_HEADER_GUARD

// input: contextual_type_reference
// output: canonical_type_reference

#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/contextual_type_reference.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"

namespace quxlang
{
    class canonical_symbol_from_contextual_symbol_resolver : public rpnx::resolver_base< compiler, type_symbol >
    {
      public:
        using key_type = contextual_type_reference;
        using input_type = contextual_type_reference;
        canonical_symbol_from_contextual_symbol_resolver(contextual_type_reference ref)
        {
            m_ref = ref;
        }

        virtual void process(compiler* c) override;

      private:
        contextual_type_reference m_ref;
    };

} // namespace quxlang

#endif // QUXLANG_CANONICAL_SYMBOL_FROM_CONTEXTUAL_SYMBOL_RESOLVER_HEADER_GUARD
