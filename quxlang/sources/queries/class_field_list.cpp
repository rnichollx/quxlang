// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_field_list_spec.hpp>

rpnx::querygraph::coroutine< quxlang::class_field_list_spec > quxlang::class_field_list_impl(type_symbol input)
{

    if (input.template type_is< readonly_constant >())
    {
        auto const &roc = as< readonly_constant >(input);
        auto field_type = ptrref_type{ .target = byte_type{}, .ptr_class = pointer_class::array, .qual = qualifier::constant };
        co_return {class_field{"__start", field_type}, class_field{"__end", field_type}};
    }
    auto declarations = co_await rpnx::querygraph::request< class_field_declaration_list_query >(input);

    std::string class_name = quxlang::to_string(input);
    std::vector< class_field > output_obj;

    for (auto& decl : declarations)
    {
        class_field f;
        contextual_type_reference type_in_context;
        type_in_context.type = decl.type;
        type_in_context.context = input;
        auto real_type = co_await rpnx::querygraph::request< lookup_query >(type_in_context);

        f.name = decl.name;
        f.type = real_type.value();

        output_obj.push_back(f);
    }

    // assert(output_obj.size() == declarations.size());

    co_return output_obj;
}