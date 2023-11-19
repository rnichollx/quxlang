#include "rylang/compiler.hpp"

#include "rylang/res/class_field_list_from_canonical_chain_resolver.hpp"


void rylang::class_field_list_from_canonical_chain_resolver::process(compiler* c)
{
    qualified_symbol_reference canonical_chain = m_chain;

    auto ast_dp = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(canonical_chain);
        });

    if (!ready())
        return;

    entity_ast ent_ast = ast_dp->get();

    if (ent_ast.type() != entity_type::class_type)
    {
        std::string type_str = to_string(canonical_chain);
        throw std::runtime_error("Cannot get layout of non-class entity: " + type_str + "");
    }

    std::vector< class_field_declaration > output;

    for (auto& [name, sub_entity_val] : ent_ast.m_sub_entities)
    {
        auto& sub_entity = sub_entity_val.get();
        if (sub_entity.m_is_field_entity && sub_entity.type() == entity_type::variable_type)
        {
            class_field_declaration f;
            variable_entity_ast const& var_data = sub_entity.get_as< variable_entity_ast >();

            f.name = name;
            f.type = var_data.m_variable_type;

            output.push_back(f);
        }
    }

    set_value(output);
}
