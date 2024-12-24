// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"

#include "quxlang/res/class_layout_resolver.hpp"

#include "quxlang/data/class_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_layout)
{

    std::string input_type_str = quxlang::to_string(input);

    class_layout output;
    // 1 get class field information

    std::vector< class_field > class_fields = co_await QUX_CO_DEP(class_field_list, (input));

    for (auto& f : class_fields)
    {

        class_field_info this_field;
        this_field.name = f.name;
        this_field.type = f.type;

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
