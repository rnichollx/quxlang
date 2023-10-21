//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/class_ast.hpp"
#include "rylang/ast/entity_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    class entity_ast_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, entity_ast >
    {

      public:
        using key_type = canonical_lookup_chain;

        explicit entity_ast_from_canonical_chain_resolver(canonical_lookup_chain chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        canonical_lookup_chain m_chain;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
