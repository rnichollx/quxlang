//
// Created by Ryan Nicholl on 11/30/23.
//

#ifndef CALLED_FUNCTANOIDS_RESOLVER_HPP
#define CALLED_FUNCTANOIDS_RESOLVER_HPP

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/vm_procedure.hpp"

#include <set>

namespace rylang
{
    class compiler;
    class called_functanoids_resolver : public rpnx::co_resolver_base< compiler, std::set< qualified_symbol_reference >, qualified_symbol_reference >
    {
      public:
        called_functanoids_resolver(qualified_symbol_reference func_addr)
            : co_resolver_base(func_addr)
        {
        }

        virtual rpnx::resolver_coroutine< compiler, std::set< qualified_symbol_reference > > co_process(compiler* c, qualified_symbol_reference func_addr) override final;
    };

} // namespace rylang
#endif // CALLED_FUNCTANOIDS_RESOLVER_HPP
