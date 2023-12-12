//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RYLANG_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
#define RYLANG_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/entity_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    class entity_ast_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, entity_ast >
    {

      public:
        using key_type = type_symbol;

        explicit entity_ast_from_canonical_chain_resolver(type_symbol chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        type_symbol m_chain;
    };

} // namespace rylang

#endif // RYLANG_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
