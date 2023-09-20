//
// Created by Ryan Nicholl on 9/19/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_SIZE_FROM_CANNONICAL_CHAIN_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_SIZE_FROM_CANNONICAL_CHAIN_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/data/lookup_chain.hpp"
#include "rpnx/graph_solver.hpp"

namespace rylang
{
    class class_size_from_cannonical_chain_resolver : public rpnx::output_base< compiler, std::size_t >
    {
        class_size_from_cannonical_chain_resolver(lookup_chain chain)
        {
                m_chain = chain;
        }

        virtual void process(compiler* c);

      private:
        lookup_chain m_chain;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_SIZE_FROM_CANNONICAL_CHAIN_RESOLVER_HEADER
