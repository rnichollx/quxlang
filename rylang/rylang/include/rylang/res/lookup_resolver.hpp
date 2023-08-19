//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_LOOKUP_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_LOOKUP_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"

#include "rpnx/graph_solver.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/data/lookup_type.hpp"
#include "rylang/data/lookup_singular.hpp"
#include "rylang/filelist.hpp"
#include <string>
namespace rylang
{
    class lookup_resolver
        : public rpnx::output_base<compiler, symbol_id>
    {
       lookup_singular m_lk;
      public:
        lookup_resolver(symbol_id from,  lookup_singular const & lk)
        : m_lk(lk)
        {
        }

        void process(compiler* c);
    };

} // namespace rylang
#endif // RPNX_RYANSCRIPT1031_LOOKUP_RESOLVER_HEADER
