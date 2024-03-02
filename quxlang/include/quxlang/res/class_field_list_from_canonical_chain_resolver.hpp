//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef QUXLANG_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
#define QUXLANG_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD

// input: canonical_lookup_chain
// output: class_field_list

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"

#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/class_field_declaration.hpp"

namespace quxlang
{
    class class_field_list_from_canonical_chain_resolver : public rpnx::resolver_base< compiler, std::vector< class_field_declaration > >
    {
      public:
        using key_type = type_symbol;
        explicit class_field_list_from_canonical_chain_resolver(type_symbol chain)
        {
            m_chain = std::move(chain);
        }

        virtual void process(compiler* c);

      private:
        type_symbol m_chain;
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
