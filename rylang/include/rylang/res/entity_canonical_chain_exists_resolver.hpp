//
// Created by Ryan Nicholl on 10/24/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/entity_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
  class entity_canonical_chain_exists_resolver : public rpnx::resolver_base< compiler, bool >
  {
    public:
        using key_type = qualified_symbol_reference;

        explicit entity_canonical_chain_exists_resolver(qualified_symbol_reference chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c);
      private:
        qualified_symbol_reference m_chain;
  };
}

#endif // RPNX_RYANSCRIPT1031_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER
