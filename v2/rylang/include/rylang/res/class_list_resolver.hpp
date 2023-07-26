//
// Created by Ryan Nicholl on 7/21/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_LIST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_LIST_RESOLVER_HEADER

#include "rpnx/graph_solver.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/class_list.hpp"

namespace rylang
{
    class class_list_resolver : public virtual rpnx::output_base< compiler, class_list >
    {
      public:
        class_list_resolver()
        {
        }

        void process(compiler* c);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_LIST_RESOLVER_HEADER
