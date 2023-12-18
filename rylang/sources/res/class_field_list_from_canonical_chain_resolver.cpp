#include "rylang/compiler.hpp"

#include "rylang/res/class_field_list_from_canonical_chain_resolver.hpp"

void rylang::class_field_list_from_canonical_chain_resolver::process(compiler* c)
{
    type_symbol canonical_chain = m_chain;

    // TODO: Check if is not a class.

    std::string name = to_string(m_chain);



    auto ast_dp = get_dependency(
        [&]
        {
            return c->lk_type_map(canonical_chain);
        });

    if (!ready())
        return;

    ast2_type_map class_map = ast_dp->get();

    std::vector< class_field_declaration > output;

    for (auto& sub_entity : class_map.members)
    {
        // TODO: check no duplicate functions with the same name as variables?
        auto name = sub_entity.first;

        if (!typeis< ast2_variable_declaration >(sub_entity.second))
        {
            // member function or something
            continue;
        }

        class_field_declaration f;
        ast2_variable_declaration const& var_data = as<ast2_variable_declaration>(sub_entity.second);

        f.name = name;
        f.type = var_data.type;

        output.push_back(f);
    }

    set_value(output);
}
