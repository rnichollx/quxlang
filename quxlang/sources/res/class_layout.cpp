//
// Created by Ryan Nicholl on 10/22/23.
//
#include "quxlang/compiler.hpp"

#include "quxlang/res/class_layout_resolver.hpp"

#include "quxlang/data/class_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_layout)
{

    class_layout output;
    // 1 get class field information

    std::vector< class_field_declaration > class_fields = co_await QUX_CO_DEP(class_field_list, (input));

    for (auto& f : class_fields)
    {
        auto fields_type = f.type;

        class_field_info this_field;
        this_field.name = f.name;

        contextual_type_reference ctx_type_ref;
        ctx_type_ref.type = fields_type;
        ctx_type_ref.context = input;

        this_field.type = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (ctx_type_ref));

        auto placement_info = co_await QUX_CO_DEP(type_placement_info_from_canonical_type, (this_field.type));

        advance_to_alignment(output.size, placement_info.alignment);

        this_field.offset = output.size;
        output.size += placement_info.size;
        output.align = std::max(output.align, placement_info.alignment);
        output.fields.push_back(this_field);
    }

    co_return output;

    // set_value(total_size);
}
