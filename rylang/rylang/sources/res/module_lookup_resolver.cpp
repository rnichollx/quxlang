//
// Created by Ryan Nicholl on 8/16/23.
//
#include "rylang/res/module_lookup_resolver.hpp"
#include "rylang/ast/module_ast_precursor1.hpp"

/** Looks up a global symbol from a named module.
 *
 * First, get the module Symbol ID.
 *
 * Then obtain the merged AST for the given module.
 *
 * Then we should look up the symbol in the merged AST.
 *
 * If we find the given symbol in the AST, assign a symbol ID to it
 * by calling the compiler and asking it for a new symbol ID.
 *
 * We should also register the symbol parent as the module.
 *
 * @param c
 */
void rylang::module_lookup_resolver::process(compiler* c)
{
    //compiler::out< symbol_id > module_id_dep = get_dependency(
    //    [&]()
    //    {
    //        return c->lk_module_symbol_id(m_module_name);
    //    });

    //if (!ready())
    //    return;
    // TODO: Support multiple modules
    //symbol_id module_id = module_id_dep->get();

    symbol_id module_id = 0;
    /*
    compiler::out< module_ast_precursor1 > module_ast_dep = get_dependency(
        [&]()
        {
            return c->lk_module_ast_precursor1(module_id);
        });

    if (!ready())
        return;

    module_ast_precursor1 v_module_ast = module_ast_dep->get();
    */
    // TODO: Suppport module precursors (e.g., for allowing IF (LINUX) inside the module definition)






}
