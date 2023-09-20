//
// Created by Ryan Nicholl on 9/19/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANNONICAL_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CANNONICAL_CHAIN_RESOLVER_HEADER

#include "rpnx/graph_solver.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    class cannonical_chain_resolver : public rpnx::output_base< compiler, lookup_chain >
    {
      public:
        using key_type = std::tuple< lookup_chain, lookup_chain >;

        explicit cannonical_chain_resolver(std::tuple< lookup_chain, lookup_chain > chain)
        {
            m_chain = std::get< 0 >(chain);
            m_context = std::get< 1 >(chain);
        }

        virtual void process(compiler* c);

      private:
        lookup_chain m_chain;
        lookup_chain m_context;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CANNONICAL_CHAIN_RESOLVER_HEADER
