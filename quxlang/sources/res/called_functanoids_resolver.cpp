// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/res/called_functanoids_resolver.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(called_functanoids)
{
    auto func_addr = input_val;
    if (!typeis< instantiation_type >(func_addr))
    {
        throw std::logic_error("Not supported or not a functanoid");
    }
    auto func_addr_inst = as< instantiation_type >(func_addr);
    vm_procedure vmf = co_await *c->lk_vm_procedure_from_canonical_functanoid(func_addr_inst);
    co_return vmf.invoked_functanoids;
}
