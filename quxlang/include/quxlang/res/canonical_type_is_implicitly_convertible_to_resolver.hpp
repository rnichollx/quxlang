//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef QUXLANG_CANONICAL_TYPE_IS_IMPLICITLY_CONVERTIBLE_TO_RESOLVER_HEADER_GUARD
#define QUXLANG_CANONICAL_TYPE_IS_IMPLICITLY_CONVERTIBLE_TO_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    class canonical_type_is_implicitly_convertible_to_resolver : public rpnx::resolver_base< compiler, bool >
    {
        type_symbol m_from;
        type_symbol m_to;

      public:
        using key_type = std::pair< type_symbol, type_symbol >;

        canonical_type_is_implicitly_convertible_to_resolver(key_type input)
            : m_from(input.first)
            , m_to(input.second)
        {
        }

        void process(compiler* c);
    };
} // namespace quxlang

#endif // QUXLANG_CANONICAL_TYPE_IS_IMPLICITLY_CONVERTIBLE_TO_RESOLVER_HEADER_GUARD
