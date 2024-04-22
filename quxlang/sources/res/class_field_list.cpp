#include "quxlang/compiler.hpp"
#include "quxlang/res/class_field_list_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_field_list)
{

    // Step 1: Ensure this is a class

    ast2_symboid the_class = co_await QUX_CO_DEP(symboid, (input_val));

    if (!typeis< ast2_class_declaration >(the_class))
    {
        throw std::logic_error("Cannot get class fields of non-class");
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

    QUX_CO_ANSWER(output);
}