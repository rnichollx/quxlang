// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_runtime_requirements_spec.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/keywords.hpp>

#include <map>
#include <set>

rpnx::querygraph::coroutine< quxlang::struct_runtime_requirements_spec > quxlang::struct_runtime_requirements_impl(type_symbol input)
{
    class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (concrete_kind != class_kind::struct_ && concrete_kind != class_kind::generic && concrete_kind != class_kind::generic_ref)
    {
        throw compiler_bug("struct_runtime_requirements received a non-STRUCT type: " + to_string(input));
    }
    ast2_symboid input_symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!input_symboid.type_is< ast2_struct_declaration >())
    {
        co_return struct_runtime_requirements{};
    }

    struct_inheritance_info inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(input);
    struct_virtual_slots slots = co_await rpnx::querygraph::request< struct_virtual_slots_query >(input);
    (void)co_await rpnx::querygraph::request< struct_constructor_forms_query >(input);
    if (input_symboid.get_as< ast2_struct_declaration >().is_ipc && inheritance.polymorphism != struct_polymorphism_kind::none)
    {
        throw semantic_compilation_error("IPC_STRUCT cannot be polymorphic: " + to_string(input));
    }

    std::map< type_symbol, struct_polymorphism_kind > category_by_type;
    for (struct_subobject_record const& subobject : inheritance.subobjects)
    {
        if (category_by_type.contains(subobject.type))
        {
            continue;
        }
        std::set< std::string > tags = co_await rpnx::querygraph::request< struct_tags_query >(subobject.type);
        struct_polymorphism_kind category = struct_polymorphism_kind::none;
        if (tags.contains(keywords::virtual_polymorphic))
        {
            category = struct_polymorphism_kind::virtual_polymorphic;
        }
        else if (tags.contains(keywords::polymorphic))
        {
            category = struct_polymorphism_kind::polymorphic;
        }
        category_by_type.emplace(subobject.type, category);
    }

    struct_runtime_requirements output;
    output.polymorphism = inheritance.polymorphism;
    output.destructor_policy = slots.destructor_policy;
    output.requires_rtti = inheritance.polymorphism != struct_polymorphism_kind::none;
    output.requires_virtual_base_navigation = inheritance.polymorphism == struct_polymorphism_kind::virtual_polymorphic;
    output.effective_destructor_is_virtual = output.requires_rtti && slots.destructor_policy == struct_destructor_policy::category_default;
    for (struct_subobject_record const& subobject : inheritance.subobjects)
    {
        if (category_by_type.at(subobject.type) != struct_polymorphism_kind::none)
        {
            output.runtime_header_subobjects.push_back(subobject.id);
        }
    }

    for (struct_base_declaration const& base : inheritance.direct_bases)
    {
        if (base.kind == inheritance_kind::virtual_)
        {
            continue;
        }
        struct_polymorphism_kind base_category = category_by_type.at(base.base_type);
        bool compatible = (inheritance.polymorphism == struct_polymorphism_kind::polymorphic && base_category == struct_polymorphism_kind::polymorphic) || (inheritance.polymorphism == struct_polymorphism_kind::virtual_polymorphic && base_category == struct_polymorphism_kind::virtual_polymorphic);
        if (compatible)
        {
            output.primary_base_candidate = struct_subobject_id{
                .nonvirtual_path = {base.declaration_ordinal},
            };
            break;
        }
    }

    co_return output;
}
