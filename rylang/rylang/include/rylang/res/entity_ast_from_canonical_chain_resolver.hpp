//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER

#include "rpnx/graph_solver.hpp"
#include "rylang/ast/class_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/lookup_chain.hpp"
#include "rylang/ast/entity_ast.hpp"

namespace rylang
{
    class entity_ast_from_canonical_chain_resolver : public rpnx::output_base< compiler, entity_ast >
    {

      public:
        using key_type = lookup_chain;

        explicit entity_ast_from_canonical_chain_resolver(lookup_chain chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        lookup_chain m_chain;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER
