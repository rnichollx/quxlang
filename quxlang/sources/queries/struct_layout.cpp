// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_layout_spec.hpp>

#include "quxlang/data/struct_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"


rpnx::querygraph::coroutine< quxlang::struct_layout_spec > quxlang::struct_layout_impl(type_symbol input)
{
    if (co_await rpnx::querygraph::request< class_type_query >(input) != class_kind::struct_)
    {
        throw compiler_bug("struct_layout received a non-struct class: " + to_string(input));
    }

    std::string input_type_str = quxlang::to_string(input);
    struct_layout output;
    // 1 get struct field information
    // TODO: Rearrange
    std::vector< struct_field > struct_fields = co_await rpnx::querygraph::request< struct_field_list_query >(input);
    for (auto& f : struct_fields)
    {
        struct_field_info this_field;
        this_field.name = f.name;
        this_field.type = f.type;
        auto placement_info = co_await rpnx::querygraph::request< class_placement_info_query >(this_field.type);
        advance_to_alignment(output.size, placement_info.alignment);
        this_field.offset = output.size;
        output.size += placement_info.size;
        output.align = std::max<std::uint64_t>(output.align, placement_info.alignment);
        output.fields.push_back(this_field);
    }
    co_return output;
}
