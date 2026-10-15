// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/list_benchmarks_spec.hpp>

rpnx::querygraph::coroutine< quxlang::list_benchmarks_spec > quxlang::list_benchmarks_impl(type_symbol input)
{
    std::vector< subdeclaration > const& input_subdeclarations = co_await rpnx::querygraph::request< active_symboid_subdeclarations_query >(input);

    std::set< type_symbol > result;

    for (subdeclaration const& sdcl : input_subdeclarations)
    {
        declaration const* decl = nullptr;
        std::string const* name = nullptr;
        bool is_member = false;

        if (sdcl.type_is< global_subdeclaration >())
        {
            global_subdeclaration const& global = sdcl.as< global_subdeclaration >();
            decl = &global.decl;
            name = &global.name;
        }
        else if (sdcl.type_is< member_subdeclaration >())
        {
            member_subdeclaration const& member = sdcl.as< member_subdeclaration >();
            decl = &member.decl;
            name = &member.name;
            is_member = true;
        }
        else
        {
            continue;
        }

        if (decl->type_is< ast2_template_declaration >() || decl->type_is< ast2_function_declaration >())
        {
            continue;
        }

        type_symbol child = is_member ? type_symbol{submember{.of = input, .name = *name}} : type_symbol{subsymbol{.of = input, .name = *name}};

        if (decl->type_is< ast2_namespace_declaration >() || decl->type_is< ast2_struct_declaration >() || decl->type_is< ast2_union_declaration >() || decl->type_is< ast2_variant_declaration >())
        {
            std::set< type_symbol > const ns_results = co_await rpnx::querygraph::request< list_benchmarks_query >(child);
            result.insert(ns_results.begin(), ns_results.end());
            continue;
        }

        if (!decl->type_is< ast2_benchmark >())
        {
            continue;
        }

        result.insert(child);
    }

    co_return result;
}
