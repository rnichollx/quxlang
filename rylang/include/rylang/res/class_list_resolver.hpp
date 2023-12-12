//
// Created by Ryan Nicholl on 7/21/23.
//

#ifndef RYLANG_CLASS_LIST_RESOLVER_HEADER_GUARD
#define RYLANG_CLASS_LIST_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/class_list.hpp"

namespace rylang
{
    class class_list_resolver : public virtual rpnx::resolver_base< compiler, class_list >
    {
      public:
        class_list_resolver()
        {
        }

        void process(compiler* c);
    };
} // namespace rylang

#endif // RYLANG_CLASS_LIST_RESOLVER_HEADER_GUARD
