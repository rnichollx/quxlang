//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER

// input: canonical_lookup_chain
// output: class_field_list

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"

#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/class_field_declaration.hpp"

namespace rylang
{
    class class_field_list_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, std::vector< class_field_declaration > >
    {
      public:
        using key_type = canonical_lookup_chain;
        class_field_list_from_canonical_chain_resolver(canonical_lookup_chain chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        canonical_lookup_chain m_chain;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
