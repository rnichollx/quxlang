//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_FILELIST_RESOLVER_HEADER_GUARD
#define RYLANG_FILELIST_RESOLVER_HEADER_GUARD
#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/filelist.hpp"
#include <string>
namespace rylang
{
    class filelist_resolver
        : public rpnx::resolver_base<compiler, filelist>
    {
      public:


        filelist_resolver()
        {
        }
        void process(compiler* c);
    };

} // namespace rylang

#endif // RYLANG_FILELIST_RESOLVER_HEADER_GUARD
