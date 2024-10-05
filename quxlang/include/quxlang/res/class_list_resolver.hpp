// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_CLASS_LIST_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_CLASS_LIST_RESOLVER_HEADER_GUARD

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
