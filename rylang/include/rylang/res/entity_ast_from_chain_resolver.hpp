//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RYLANG_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER_GUARD
#define RYLANG_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/lookup_chain.hpp"

#include <rylang/ast2/ast2_entity.hpp>

namespace rylang
{
    class entity_ast_from_chain_resolver : public rpnx::resolver_base< compiler, ast2_map_entity >
    {

      public:
        using key_type = std::tuple< type_symbol, type_symbol >;

        explicit entity_ast_from_chain_resolver(key_type chains)
        {
            m_chain = std::get< 0 >(chains);
            m_context = std::get< 1 >(chains);
        }

        virtual void process(compiler* c);

      private:
        type_symbol m_chain;
        type_symbol m_context;
    };
} // namespace rylang

#endif // RYLANG_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER_GUARD
