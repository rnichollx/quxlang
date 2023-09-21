//
// Created by Ryan Nicholl on 8/11/23.
//
#include "rylang/res/class_member_variable_list_resolver.hpp"
#include "rylang/ast/class_entity_ast.hpp"
#include "rylang/compiler.hpp"

/** This function first gets a list of the precursors, resolves them to class IDs,
 * then each class ID size is returned as the class member variable list.
 * @param c compiler
 */
void rylang::class_member_variable_list_resolver::process(compiler* c)
{
    auto ast_dep = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(m_id);
        });

    if (!ready())
        return;

    entity_ast ast = ast_dep->get();

    if (!std::holds_alternative< class_entity_ast >(ast.m_subvalue.get()))
    {
        try
        {
            throw std::runtime_error("Tried to get class member variable list of non-class entity");
        }
        catch (...)
        {
            set_error(std::current_exception());
        }
        // set_value(class_member_variable_list());
        return;
    }

    class_entity_ast const& class_ast = std::get< class_entity_ast >(ast.m_subvalue.get());
    class_member_variable_list output;
    for (auto const& entity_kv : ast.m_sub_entities)
    {
        std::string const& entity_name = entity_kv.first;
        entity_ast const& entity = entity_kv.second.get();

        if (entity.m_is_field_entity && std::holds_alternative< variable_entity_ast >(entity.m_subvalue.get()))
        {
            variable_entity_ast const& var = std::get< variable_entity_ast >(entity.m_subvalue.get());

            class_member_variable_declaration decl;
            decl.name = entity_name;
            decl.typeref = var.m_variable_type;

            output.member_variables.push_back(decl);
        }
    }

    set_value(output);
}
