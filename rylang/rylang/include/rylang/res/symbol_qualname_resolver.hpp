//
// Created by Ryan Nicholl on 8/16/23.
//

#ifndef RPNX_RYANSCRIPT1031_SYMBOL_QUALNAME_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_SYMBOL_QUALNAME_RESOLVER_HEADER

#include "rpnx/graph_solver.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    class symbol_qualname_resolver : public rpnx::output_base< compiler, filelist >
    {
      public:
        symbol_qualname_resolver()
        {
        }
        void process(compiler* c);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_SYMBOL_QUALNAME_RESOLVER_HEADER
