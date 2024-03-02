//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef QUXLANG_TYPE_SIZE_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
#define QUXLANG_TYPE_SIZE_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_type_reference.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"

namespace quxlang
{
    class type_size_from_canonical_type_resolver
    : public rpnx::resolver_base< compiler, std::size_t >
    {
      public:
        using key_type = type_symbol;

        virtual void process(compiler* c) override;

        type_size_from_canonical_type_resolver(type_symbol type)
        {
            m_type = type;
        }
      private:
        type_symbol m_type;

    };
} // namespace quxlang

#endif // QUXLANG_TYPE_SIZE_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
