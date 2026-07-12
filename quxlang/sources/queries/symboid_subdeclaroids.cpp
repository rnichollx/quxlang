// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/symboid_subdeclaroids_spec.hpp>
// This file implements the symboid_sub_declaroids resolver.



rpnx::querygraph::coroutine< quxlang::symboid_subdeclaroids_spec > quxlang::symboid_subdeclaroids_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);

    if (typeis< ast2_struct_declaration >(sym))
    {
        co_return as< ast2_struct_declaration >(sym).declarations;
    }
    else if (typeis< ast2_union_declaration >(sym))
    {
        co_return as< ast2_union_declaration >(sym).declarations;
    }
    else if (typeis< ast2_variant_declaration >(sym))
    {
        co_return as< ast2_variant_declaration >(sym).declarations;
    }
    else if (typeis< ast2_implementation_declaration >(sym))
    {
        co_return as< ast2_implementation_declaration >(sym).declarations;
    }
    else if (typeis< ast2_enum_declaration >(sym))
    {
        co_return as< ast2_enum_declaration >(sym).declarations;
    }
    else if (typeis< ast2_flagset_declaration >(sym))
    {
        co_return as< ast2_flagset_declaration >(sym).declarations;
    }
    else if (typeis< ast2_module_declaration >(sym))
    {
        co_return as< ast2_module_declaration >(sym).declarations;
    }
    else if (typeis< ast2_namespace_declaration >(sym))
    {
        co_return as< ast2_namespace_declaration >(sym).declarations;
    }
    else if (typeis< ast2_template_declaration >(sym))
    {
        // Templates don't have subdeclaroids, only a template instanciation could,
        // but that would produce a class, not a template.
        // e.g. ::foo#(I32) would produce a class, not a template.
        // whereas ::foo doesn't have any subdeclaroids, even if ::foo#(I32) does.
        co_return {};
    }
    else if (typeis<functum>(sym))
    {
        co_return {};
    }
    else
    {
       co_return {};
    }
}
