//
// Created by Ryan Nicholl on 10/22/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/class_layout_from_canonical_chain_resolver.hpp"

#include "rylang/data/class_field_declaration.hpp"
#include "rylang/manipulators/struct_math.hpp"


void rylang::class_layout_from_canonical_chain_resolver::process(compiler* c)
{
    auto& chain = this->m_chain;
    std::string class_str = to_string(chain);

    class_layout output;
    // 1 get class field information
    auto class_field_dp = get_dependency(
        [&]
        {
            return c->lk_class_field_declaration_list_from_canonical_chain(chain);
        });
    if (!ready())
        return;

    std::vector< class_field_declaration > class_fields = class_field_dp->get();

    for (auto& f : class_fields)
    {
        auto fields_type = f.type;

        class_field_info this_field;
        this_field.name = f.name;

        contextual_type_reference ctx_type_ref;
        ctx_type_ref.type = fields_type;
        ctx_type_ref.context = chain;

        auto canonical_type_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_type_from_contextual_type(ctx_type_ref);
            });

        if (!ready())
            return;

        this_field.type = canonical_type_dp->get();

        auto placement_info_dp = get_dependency(
            [&]
            {
                return c->lk_type_placement_info_from_canonical_type(this_field.type);
            });

        if (!ready())
            return;

        auto placement_info = placement_info_dp->get();


       advance_to_alignment(output.size, placement_info.alignment);

        this_field.offset = output.size;
        output.size += placement_info.size;
        output.align = std::max(output.align, placement_info.alignment);
        output.fields.push_back(this_field);
    }

    set_value(output);

    //set_value(total_size);
}
