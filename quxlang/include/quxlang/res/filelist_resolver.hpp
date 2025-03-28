// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_FILELIST_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_FILELIST_RESOLVER_HEADER_GUARD
#include "quxlang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/filelist.hpp"
#include <string>
namespace quxlang
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

} // namespace quxlang

#endif // QUXLANG_FILELIST_RESOLVER_HEADER_GUARD
