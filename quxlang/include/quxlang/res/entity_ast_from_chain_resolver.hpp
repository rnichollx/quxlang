// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/lookup_chain.hpp"

#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    class entity_ast_from_chain_resolver : public rpnx::resolver_base< compiler, ast2_node >
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
} // namespace quxlang

#endif // QUXLANG_ENTITY_AST_FROM_CHAIN_RESOLVER_HEADER_GUARD
