//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RYLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
#define RYLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/data/type_placement_info.hpp"

namespace rylang
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
} // namespace rylang

#endif // RYLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
