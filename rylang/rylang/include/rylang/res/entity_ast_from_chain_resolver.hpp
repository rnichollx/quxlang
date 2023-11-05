//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/class_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    class entity_ast_from_chain_resolver : public rpnx::resolver_base< compiler, entity_ast >
    {

      public:
        using key_type = std::tuple< qualified_symbol_reference , qualified_symbol_reference >;

        explicit entity_ast_from_chain_resolver(key_type chains)
        {
            m_chain = std::get<0>(chains);
            m_context = std::get<1>(chains);
        }

        virtual void process(compiler* c);

      private:
        qualified_symbol_reference m_chain;
        qualified_symbol_reference m_context;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER
