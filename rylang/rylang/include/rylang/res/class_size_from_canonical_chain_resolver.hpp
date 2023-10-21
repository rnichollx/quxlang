//
// Created by Ryan Nicholl on 9/19/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_SIZE_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_SIZE_FROM_CANONICAL_CHAIN_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/symbol_id.hpp"

namespace rylang
{
    class class_size_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, std::size_t >
    {
      public:
      using key_type = canonical_lookup_chain;
        class_size_from_canonical_chain_resolver(canonical_lookup_chain chain)
        {
                m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        canonical_lookup_chain m_chain;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_SIZE_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
