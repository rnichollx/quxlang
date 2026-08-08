// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/declaroids_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::declaroids_spec > quxlang::declaroids_impl(type_symbol input)
{
    std::vector< declaroid > output;

    std::string inputname = to_string(input);

    if (typeis< absolute_module_reference >(input))
    {
        throw quxlang::compiler_bug("Cannot have declarations of a module");
    }

    if (typeis< initialization_reference >(input))
    {
        throw quxlang::compiler_bug("Non-canonical symbol passed to declaroids resolver: initialization_reference. Canonicalize with lookup/instanciation before calling declaroids.");
    }

    if (!typeis< subsymbol >(input) && !typeis< submember >(input))
    {
        co_return {};
    }

    std::vector< subdeclaroid > const& subdeclaroids = co_await rpnx::querygraph::request< active_subdeclaroids_query >(input);

    for (subdeclaroid const& subdecl : subdeclaroids)
    {
        if (typeis< member_subdeclaroid >(subdecl))
        {
            member_subdeclaroid const& member = as< member_subdeclaroid >(subdecl);
            output.push_back(member.decl);
        }
        else
        {
            global_subdeclaroid const& global = as< global_subdeclaroid >(subdecl);
            output.push_back(global.decl);
        }
    }

    co_return output;
}
