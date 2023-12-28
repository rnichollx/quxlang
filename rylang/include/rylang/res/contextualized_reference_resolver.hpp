//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RYLANG_CONTEXTUALIZED_REFERENCE_RESOLVER_HEADER_GUARD
#define RYLANG_CONTEXTUALIZED_REFERENCE_RESOLVER_HEADER_GUARD

#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class contextualized_reference_resolver : public rpnx::resolver_base< compiler, type_symbol >
    {
      private:
        type_symbol m_symbol;
        type_symbol m_context;

      public:
        using key_type = std::pair< type_symbol, type_symbol >;

        contextualized_reference_resolver(key_type input)
            : m_symbol(input.first)
            , m_context(input.second)
        {
        }
        void process(compiler* c);
    };
} // namespace rylang

#endif // RYLANG_CONTEXTUALIZED_REFERENCE_RESOLVER_HEADER_GUARD
