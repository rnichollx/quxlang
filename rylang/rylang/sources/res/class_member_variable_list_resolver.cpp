//
// Created by Ryan Nicholl on 8/11/23.
//
#include "rylang/res/class_member_variable_list_resolver.hpp"

/** This function first gets a list of the precursors, resolves them to class IDs,
 * then each class ID size is returned as the class member variable list.
 * @param c compiler
 */
void rylang::class_member_variable_list_resolver::process(compiler* c)
{
    // Each precursor can be determined from the class's AST.
    // TODO

    /*
    auto my_class_ast_dep = get_dependency(
        [&]()
        {
            return c->lk_class_ast(m_id);
        });

    if (!ready())
        return;

    auto my_class_ast = my_class_ast_dep->get();


    for (auto const& member: my_class_ast.member_variables)
    {
       auto const& type_ref = member.type;
       // TODO
               assert(false);
        //auto type_id_dep = get_dependency(
       //     [&]()
       //     {
                 //return c->lk_symbol_id(type_ref);
       //     });
    }
     */
}
