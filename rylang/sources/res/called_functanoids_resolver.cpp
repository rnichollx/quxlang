// Created by Ryan Nicholl on 11/30/23.
//


#include "rylang/res/called_functanoids_resolver.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    rpnx::resolver_coroutine< compiler, std::set< type_symbol > > called_functanoids_resolver::co_process(compiler* c, type_symbol func_addr)
    {
        vm_procedure vmf = co_await *c->lk_vm_procedure_from_canonical_functanoid(func_addr);
        co_return vmf.invoked_functanoids;
    }
}