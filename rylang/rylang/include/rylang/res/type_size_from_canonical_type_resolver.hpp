//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_SIZE_FROM_CANONICAL_TYPE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_SIZE_FROM_CANONICAL_TYPE_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_type_reference.hpp"

#include "rpnx/resolver_utilities.hpp"

namespace rylang
{
    class type_size_from_canonical_type_resolver
    : public rpnx::resolver_base< compiler, std::size_t >
    {
      public:
        using key_type = canonical_type_reference;

        virtual void process(compiler* c) override;

        type_size_from_canonical_type_resolver(canonical_type_reference type)
        {
            m_type = type;
        }
      private:
        canonical_type_reference m_type;

    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_TYPE_SIZE_FROM_CANONICAL_TYPE_RESOLVER_HEADER
