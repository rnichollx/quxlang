//
// Created by Ryan Nicholl on 10/24/23.
//

#ifndef QUXLANG_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD
#define QUXLANG_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/ast/entity_ast.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/lookup_chain.hpp"

namespace quxlang
{
  class entity_canonical_chain_exists_resolver : public rpnx::resolver_base< compiler, bool >
  {
    public:
        using key_type = type_symbol;

        explicit entity_canonical_chain_exists_resolver(type_symbol chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c) override;

        virtual std::string question() const override
        {
            return "entity_canonical_chain_exists(" + to_string(m_chain) + ")";
        }
      private:
        type_symbol m_chain;
  };
}

#endif // QUXLANG_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD
