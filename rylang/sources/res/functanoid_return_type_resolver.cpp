//
// Created by Ryan Nicholl on 11/23/23.
//
#include "rylang/res/functanoid_return_type_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::functanoid_return_type_resolver::process(rylang::compiler* c)
{

    auto functanoid_name = m_function_name;

    assert(!is_contextual(functanoid_name));

    auto parent = functanoid_name.callee;

    // We should shortcut CONSTRUCTOR and DESTRUCTOR subentity types to return void always
    if (parent.type() == boost::typeindex::type_id< subdotentity_reference >())
    {
        subdotentity_reference subdot = as<subdotentity_reference>(parent);
        if (subdot.subdotentity_name == "CONSTRUCTOR" || subdot.subdotentity_name == "DESTRUCTOR")
        {
            set_value(type_symbol{void_type{}});
            return;
        }
    }

    auto ast_dp = get_dependency(
        [&]
        {
            return c->lk_function_ast(functanoid_name);
        });

    if (!ready())
    {
        return;
    }

    function_ast func_ast = ast_dp->get();

    contextual_type_reference return_type_contexual;
    return_type_contexual.type = func_ast.return_type.value_or(void_type{});
    return_type_contexual.context = functanoid_name;

    auto return_type_dp = get_dependency(
        [&]
        {
            return c->lk_canonical_type_from_contextual_type(return_type_contexual);
        });

    if (!ready())
    {
        return;
    }

    type_symbol return_type = return_type_dp->get();
    set_value(return_type);
}
