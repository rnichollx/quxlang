//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RYLANG_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
#define RYLANG_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD

// input: canonical_lookup_chain
// output: class_field_list

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"

#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/class_field_declaration.hpp"

namespace rylang
{
    class class_field_list_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, std::vector< class_field_declaration > >
    {
      public:
        using key_type = type_symbol;
        class_field_list_from_canonical_chain_resolver(type_symbol chain)
        {
            m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        type_symbol m_chain;
    };
} // namespace rylang

#endif // RYLANG_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
