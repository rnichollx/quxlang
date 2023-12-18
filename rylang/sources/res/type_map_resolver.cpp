#include <rylang/ast2/ast2_type_map.hpp>
#include <rylang/manipulators/merge_entity.hpp>
#include <rylang/res/type_map_resolver.hpp>
//
// Created by Ryan Nicholl on 12/12/23.
//
rpnx::resolver_coroutine< rylang::compiler, rylang::ast2_type_map > rylang::type_map_resolver::co_process(compiler* c, key_type input)
{

    std::vector< std::pair< std::string, ast2_declarable > > ast_members;
    std::vector< std::pair< std::string, ast2_declarable > > ast_globals;

    auto ast = co_await *c->lk_entity_ast_from_canonical_chain(input);

    if (typeis< ast2_class_declaration >(ast))
    {
        ast_members = boost::get< ast2_class_declaration >(ast).members;
        ast_globals = boost::get< ast2_class_declaration >(ast).globals;
    }
    else if (typeis< ast2_module_declaration >(ast))
    {
        ast_globals = boost::get< ast2_module_declaration >(ast).globals;
    }
    else if (typeis< ast2_namespace_declaration >(ast))
    {
        ast_globals = boost::get< ast2_namespace_declaration >(ast).globals;
    }
    else {}

    output_type output;

    for (std::size_t i = 0; i < ast_members.size(); i++)
    {
        std::string name = ast_members[i].first;
        ast2_declarable const& ent_decl = ast_members[i].second;

        merge_entity(output.members[name], ent_decl);
    }

    for (std::size_t i = 0; i < ast_globals.size(); i++)
    {
        std::string name = ast_globals[i].first;
        ast2_declarable const& ent_decl = ast_globals[i].second;

        merge_entity(output.globals[name], ent_decl);
    }

    co_return output;
}
