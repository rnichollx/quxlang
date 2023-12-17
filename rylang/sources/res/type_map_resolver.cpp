#include <rylang/ast2/ast2_type_map.hpp>
#include <rylang/res/type_map_resolver.hpp>
//
// Created by Ryan Nicholl on 12/12/23.
//
rpnx::resolver_coroutine< rylang::compiler, rylang::ast2_type_map > rylang::type_map_resolver::co_process(compiler* c, key_type input)
{

    std::vector< std::pair< std::string, ast2_declaration > > ast_members;
    std::vector< std::pair< std::string, ast2_declaration > > ast_globals;

    auto ast = co_await *c->lk_entity_ast_from_canonical_chain(input);

    if (typeis< ast2_class_declaration >(ast))
    {
        ast_members = boost::get< ast2_class_declaration >(ast).members;
    }
    else
    {
        // TODO: Consider if this should be an exception or not.
        co_return {};
        // throw std::runtime_error("Cannot get member map of non-class entity");
    }

    std::map< std::string, rylang::ast2_map_entity > output;

    for (std::size_t i = 0; i < ast_members.size(); i++)
    {
        std::string name = ast_members[i].first;
        ast2_declaration const& ent_decl = ast_members[i].second;

        if (typeis< ast2_class_declaration >(ent_decl))
        {
            auto ent_ast = boost::get< ast2_class_declaration >(ent_decl);
            auto it = output.find(name);
            if (it == output.end())
            {
                output[name] = ast2_class_declaration{};
            }
            else
            {
                throw std::runtime_error("Cannot redeclare " + name + " as class");
            }
        }
        else if (typeis< ast2_function_declaration >(ent_decl))
        {
            auto func_ast = boost::get< ast2_function_declaration >(ent_decl);
            auto it = output.find(name);
            if (it == output.end())
            {
                output[name] = ast2_functum{};
            }
            if (!typeis< ast2_functum >(output[name]))
            {
                throw std::runtime_error("Cannot redeclare " + name + " as function");
            }
            auto& functum_ast = boost::get< ast2_functum >(output[name]);
            functum_ast.functions.push_back(func_ast);
            // TODO: include if reference.
        }
        else if (typeis< ast2_variable_declaration >(ent_decl))
        {
            auto var_ast = boost::get< ast2_variable_declaration >(ent_decl);
            auto it = output.find(name);
            if (it != output.end())
            {
                throw std::runtime_error("Cannot redeclare " + name + " as variable");
            }
            // TODO: Check include if
            output[name] = var_ast;
        }
    }

    co_return output;
}