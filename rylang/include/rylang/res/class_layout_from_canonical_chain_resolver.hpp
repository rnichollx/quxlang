//
// Created by Ryan Nicholl on 9/19/23.
//

#ifndef RYLANG_CLASS_LAYOUT_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
#define RYLANG_CLASS_LAYOUT_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/class_layout.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/symbol_id.hpp"

namespace rylang
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
} // namespace rylang

#endif // RYLANG_CLASS_SIZE_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
