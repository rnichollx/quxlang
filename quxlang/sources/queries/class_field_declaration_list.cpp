// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/class_field_declaration_list_spec.hpp>
#include <quxlang/data/lambda_types.hpp>

rpnx::querygraph::coroutine< quxlang::class_field_declaration_list_spec > quxlang::class_field_declaration_list_impl(type_symbol input)
{

    std::string name = quxlang::to_string(input);

    if (auto lambda = parse_lambda_closure_symbol(input); lambda.has_value())
    {
        auto captures = co_await rpnx::querygraph::subquery_request< lambda_capture_set_subquery >(as< instanciation_reference >(lambda->parent_functanoid), lambda->index);
        std::vector< class_field_declaration > output;
        for (std::size_t i = 0; i < captures.size(); i++)
        {
            output.push_back(class_field_declaration{
                .name = lambda_capture_field_name(i),
                .type = captures.at(i),
            });
        }
        co_return output;
    }

    if (input.template type_is< readonly_constant >())
    {
        assert(false);
    }

    bool is_builtin_class = co_await rpnx::querygraph::request< class_builtin_query >(input);
    if (is_builtin_class)
    {
        co_return {};
    }
    ast2_symboid the_class = co_await rpnx::querygraph::request< symboid_query >(input);

    if (typeis< ast2_interface_declaration >(the_class) || typeis< ast2_implementation_declaration >(the_class))
    {
        co_return {};
    }

    if (!typeis< ast2_class_declaration >(the_class))
    {
        throw quxlang::compiler_bug("Cannot get class fields of non-class");
    }
    ast2_class_declaration const& class_obj = as< ast2_class_declaration >(the_class);

    std::vector< class_field_declaration > output;

    // Step 2: get all the member declarations

    // TODO: Support include_if here by getting something like, subdeclaroids_filtered?
    std::vector< subdeclaroid > const& decls = class_obj.declarations;

    for (auto& decl : decls)
    {
        // Filter out static members
        if (!typeis< member_subdeclaroid >(decl))
        {
            // Filter out globals here
            continue;
        }

        member_subdeclaroid const& member_decl = as< member_subdeclaroid >(decl);

        // Filter out functions and etc.
        if (!typeis< ast2_variable_declaration >(member_decl.decl))
        {
            // member function or something
            continue;
        }

        class_field_declaration f;
        ast2_variable_declaration const& var_data = as< ast2_variable_declaration >(member_decl.decl);

        f.name = member_decl.name;
        f.type = var_data.type;

        output.push_back(f);
    }

    co_return output;
}
