//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef QUXLANG_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
#define QUXLANG_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/ast/entity_ast.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/lookup_chain.hpp"

#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    class entity_ast_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, ast2_node >
    {

      public:
        using key_type = type_symbol;
        using input_type = type_symbol;

        explicit entity_ast_from_canonical_chain_resolver(type_symbol chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c) override;

        virtual std::string question() const override
        {
            return "entity_ast_from_canonical_chain(" + to_string(m_chain) + ")";
        }

      private:
        type_symbol m_chain;
    };

} // namespace quxlang

#endif // QUXLANG_ENTITY_AST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
