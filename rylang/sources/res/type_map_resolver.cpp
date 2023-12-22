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

    std::string inputname = to_string(input);

    ast2_node ast = co_await *c->lk_entity_ast_from_canonical_chain(input);

start:

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

    else if (typeis< ast2_templex >(ast))
    {
        if (typeis< instanciation_reference >(input))
        {
            ast2_node ast2 = co_await *c->lk_temploid_instanciation(as< instanciation_reference >(input));

            // do stuff

            assert(!typeis<ast2_templex>(ast2));
            goto start;
            //    instanciation_reference inst = as<instanciation_reference
        }
        // We need to instanciate the template in this case

        // wut?
        co_return {};
    }
    else if (typeis< ast2_functum >(ast))
    {
        // ignore this
    }
    else
    {
        std::string typenam = to_string(input);
        std::string nametp = ast.type().name();
        rpnx::unimplemented();
    }

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
