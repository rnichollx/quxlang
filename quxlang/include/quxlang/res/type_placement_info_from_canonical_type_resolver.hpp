//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef QUXLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
#define QUXLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/symbol_id.hpp"
#include "quxlang/data/type_placement_info.hpp"

namespace quxlang
{
    class type_placement_info_from_canonical_type_resolver : public rpnx::resolver_base< compiler, type_placement_info >
    {
      public:
        using key_type = type_symbol;
        type_placement_info_from_canonical_type_resolver(type_symbol type)
        {
            m_type = type;
        }

        virtual void process(compiler* c);

      private:
        type_symbol m_type;
    };
} // namespace quxlang

#endif // QUXLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
