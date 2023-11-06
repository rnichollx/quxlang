//
// Created by Ryan Nicholl on 11/5/23.
//
#include "rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/manipulators/qualified_reference.hpp"
#include "rylang/variant_utils.hpp"

void rylang::vm_procedure_from_canonical_functanoid_resolver::process(compiler* c)
{
    auto function_ast_dp = get_dependency(
        [&]
        {
            return c->lk_function_ast(m_func_name);
        });

    if (!ready())
    {
        return;
    }

    function_ast function_ast_v = function_ast_dp->get();

    vm_procedure vm_proc;




}
