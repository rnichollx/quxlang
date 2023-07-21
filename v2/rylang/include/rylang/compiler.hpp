//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/filelist.hpp"

namespace rylang
{
    class compiler
    {
      public:
        compiler();

        filelist get_file_list();
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
