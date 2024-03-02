//
// Created by Ryan Nicholl on 7/21/23.
//

#ifndef QUXLANG_CLASS_LIST_RESOLVER_HEADER_GUARD
#define QUXLANG_CLASS_LIST_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_list.hpp"

namespace quxlang
{
    class class_list_resolver : public virtual rpnx::resolver_base< compiler, class_list >
    {
      public:
        class_list_resolver()
        {
        }

        void process(compiler* c);
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_LIST_RESOLVER_HEADER_GUARD
