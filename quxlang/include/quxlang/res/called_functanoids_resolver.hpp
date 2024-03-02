//
// Created by Ryan Nicholl on 11/30/23.
//

#ifndef CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD
#define CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"
#include "quxlang/data/vm_procedure.hpp"

#include <set>

namespace quxlang
{
    class compiler;
    class called_functanoids_resolver : public rpnx::co_resolver_base< compiler, std::set< type_symbol >, type_symbol >
    {
      public:
        called_functanoids_resolver(type_symbol func_addr)
            : co_resolver_base(std::move(func_addr))
        {
        }

        virtual rpnx::resolver_coroutine< compiler, std::set< type_symbol > > co_process(compiler* c, type_symbol func_addr) override final;
    };

} // namespace quxlang
#endif // CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD
