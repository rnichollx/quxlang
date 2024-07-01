// Created by Ryan Nicholl on 11/30/23.
//

#include "quxlang/res/called_functanoids_resolver.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(called_functanoids)
{
    auto func_addr = input_val;
    if (!typeis< instanciation_reference >(func_addr))
    {
        throw std::logic_error("Not supported or not a functanoid");
    }
    auto func_addr_inst = as< instanciation_reference >(func_addr);
    vm_procedure vmf = co_await *c->lk_vm_procedure_from_canonical_functanoid(func_addr_inst);
    co_return vmf.invoked_functanoids;
}
