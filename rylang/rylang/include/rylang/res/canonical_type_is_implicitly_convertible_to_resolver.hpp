//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANONICAL_TYPE_IS_IMPLICITLY_CONVERTIBLE_TO_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CANONICAL_TYPE_IS_IMPLICITLY_CONVERTIBLE_TO_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_type_reference.hpp"

namespace rylang
{
    class canonical_type_is_implicitly_convertible_to_resolver : public rpnx::resolver_base< compiler, bool >
    {
        canonical_type_reference from;
        canonical_type_reference to;

      public:
        using key_type = std::pair< canonical_type_reference, canonical_type_reference >;

        canonical_type_is_implicitly_convertible_to_resolver(key_type input)
            : from(input.first)
            , to(input.second)
        {
        }

        void process(compiler* c);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CANONICAL_TYPE_IS_IMPLICITLY_CONVERTIBLE_TO_RESOLVER_HEADER
