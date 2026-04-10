// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_layout_spec.hpp>

#include "quxlang/data/class_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"


rpnx::querygraph::coroutine< quxlang::class_layout_spec > quxlang::class_layout_impl(type_symbol input)
{
    std::string input_type_str = quxlang::to_string(input);
    class_layout output;
    // 1 get class field information
    // TODO: Rearrange
    std::vector< class_field > class_fields = co_await rpnx::querygraph::request< class_field_list_query >(input);
    for (auto& f : class_fields)
    {
        class_field_info this_field;
        this_field.name = f.name;
        this_field.type = f.type;
        auto placement_info = co_await rpnx::querygraph::request< type_placement_info_query >(this_field.type);
        advance_to_alignment(output.size, placement_info.alignment);
        this_field.offset = output.size;
        output.size += placement_info.size;
        output.align = std::max<std::uint64_t>(output.align, placement_info.alignment);
        output.fields.push_back(this_field);
    }
    co_return output;
}
