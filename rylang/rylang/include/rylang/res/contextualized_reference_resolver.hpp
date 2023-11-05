//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_CONTEXTUALIZED_REFERENCE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CONTEXTUALIZED_REFERENCE_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    struct contextualized_reference_resolver : public rpnx::resolver_base< compiler, qualified_symbol_reference >
    {
      private:
        qualified_symbol_reference m_symbol;
        qualified_symbol_reference m_context;

      public:
        using key_type = std::pair< qualified_symbol_reference, qualified_symbol_reference >;

        contextualized_reference_resolver(key_type input)
            : m_symbol(input.first)
            , m_context(input.second)
        {
        }
        void process(compiler* c);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CONTEXTUALIZED_REFERENCE_RESOLVER_HEADER
