//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER

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
        using key_type = qualified_symbol_reference;

        explicit entity_ast_from_canonical_chain_resolver(qualified_symbol_reference chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        qualified_symbol_reference m_chain;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
