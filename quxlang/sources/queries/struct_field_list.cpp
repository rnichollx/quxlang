// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_field_list_spec.hpp>
#include <quxlang/manipulators/declaration_alignment.hpp>

rpnx::querygraph::coroutine< quxlang::struct_field_list_spec > quxlang::struct_field_list_impl(type_symbol input)
{

    if (input.template type_is< readonly_constant >())
    {
        auto field_type = ptrref_type{ .target = byte_type{}, .ptr_class = pointer_class::array, .qual = qualifier::constant };
        co_return {struct_field{"__start", field_type}, struct_field{"__end", field_type}};
    }
    auto declarations = co_await rpnx::querygraph::request< struct_field_declaration_list_query >(input);

    std::string struct_name = quxlang::to_string(input);
    std::vector< struct_field > output_obj;

    for (auto& decl : declarations)
    {
        struct_field f;
        contextual_type_reference type_in_context;
        type_in_context.type = decl.type;
        type_in_context.context = input;
        auto real_type = co_await rpnx::querygraph::request< lookup_query >(type_in_context);

        f.name = decl.name;
        f.type = real_type.value();
        f.ibc_access = decl.ibc_access;
        if (decl.alignment.has_value())
        {
            std::uint64_t value = co_await rpnx::querygraph::request< constexpr_u64_query >(constexpr_input{.expr = decl.alignment->value, .context = input});
            f.minimum_alignment = declaration_alignment_bytes(value, decl.alignment->is_exponent);
        }

        output_obj.push_back(f);
    }

    // assert(output_obj.size() == declarations.size());

    co_return output_obj;
}
