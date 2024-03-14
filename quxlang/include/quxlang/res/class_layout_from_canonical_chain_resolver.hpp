//
// Created by Ryan Nicholl on 9/19/23.
//

#ifndef QUXLANG_CLASS_LAYOUT_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
#define QUXLANG_CLASS_LAYOUT_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/symbol_id.hpp"

namespace quxlang
{
    class class_layout_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, class_layout >
    {
      public:
        using key_type = type_symbol;
        class_layout_from_canonical_chain_resolver(type_symbol chain)
        {
           m_chain = chain;
        }

        virtual void process(compiler* c);

        std::string question() const override
        {
            return "class_layout_from_canonical_chain(" + to_string(m_chain) + ")";
        }

      private:
        type_symbol m_chain;
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_SIZE_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
