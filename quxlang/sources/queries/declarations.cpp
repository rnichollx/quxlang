// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/declarations_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::declarations_spec > quxlang::declarations_impl(type_symbol input)
{
    std::vector< declaration > output;

    std::string inputname = to_string(input);

    if (typeis< absolute_module_reference >(input))
    {
        throw quxlang::compiler_bug("Cannot have declarations of a module");
    }

    if (typeis< initialization_reference >(input))
    {
        throw quxlang::compiler_bug("Non-canonical symbol passed to declarations resolver: initialization_reference. Canonicalize with lookup/instanciation before calling declarations.");
    }

    if (!typeis< subsymbol >(input) && !typeis< submember >(input))
    {
        co_return {};
    }

    std::vector< subdeclaration > const& subdeclarations = co_await rpnx::querygraph::request< active_subdeclarations_query >(input);

    for (subdeclaration const& subdecl : subdeclarations)
    {
        if (typeis< member_subdeclaration >(subdecl))
        {
            member_subdeclaration const& member = as< member_subdeclaration >(subdecl);
            output.push_back(member.decl);
        }
        else
        {
            global_subdeclaration const& global = as< global_subdeclaration >(subdecl);
            output.push_back(global.decl);
        }
    }

    co_return output;
}
