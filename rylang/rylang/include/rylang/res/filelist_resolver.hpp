//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FILELIST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FILELIST_RESOLVER_HEADER
#include "rylang/compiler_fwd.hpp"

#include "rpnx/graph_solver.hpp"
#include <string>
#include "rylang/filelist.hpp"
namespace rylang
{
    class filelist_resolver
        : public rpnx::output_base<compiler, filelist>
    {
      public:


        filelist_resolver()
        {
        }
        void process(compiler* c);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FILELIST_RESOLVER_HEADER
