//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/data/type_placement_info.hpp"
#include "rylang/data/canonical_type_reference.hpp"

namespace rylang
{
    class type_placement_info_from_canonical_type_resolver : public rpnx::resolver_base< compiler, type_placement_info >
    {
      public:
        using key_type = canonical_type_reference;
        type_placement_info_from_canonical_type_resolver(canonical_type_reference type)
        {
            m_type = type;
        }

        virtual void process(compiler* c);

      private:
        canonical_type_reference m_type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER
