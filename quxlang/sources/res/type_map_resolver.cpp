#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/manipulators/merge_entity.hpp>
#include <quxlang/res/type_map_resolver.hpp>
#include <quxlang/compiler.hpp>
//
// Created by Ryan Nicholl on 12/12/23.
//
rpnx::resolver_coroutine< quxlang::compiler, quxlang::ast2_type_map > quxlang::type_map_resolver::co_process(compiler* c, key_type input)
{

    std::vector< ast2_named_declaration > ast_declarations;

    std::string inputname = to_string(input);

    ast2_node ast = co_await *c->lk_entity_ast_from_canonical_chain(input);

start:

    auto include_decl = [&](std::vector< ast2_top_declaration > const& decls) -> rpnx::general_coroutine< compiler, void >
    {
        for (auto const& decl : decls)
        {
            if (typeis< ast2_include_if >(decl))
            {
                auto& inc = as< ast2_include_if >(decl);
                expr_interp_input i_input{.context = input, .expression = inc.condition};

                bool should_include = true;//co_await *c->lk_interpret_bool(i_input);
                // TODO

                if (should_include)
                {
                    ast_declarations.push_back(inc.declaration);
                }
            }
            else if (typeis< ast2_named_declaration >(decl))
            {
                ast_declarations.push_back(as< ast2_named_declaration >(decl));
                // ast_globals.push_back(as< ast2_named_declaration >(decl));
            }
        }
        co_return;
    };

    if (typeis< ast2_class_declaration >(ast))
    {
        co_await include_decl(as< ast2_class_declaration >(ast).declarations);
    }
    else if (typeis< ast2_module_declaration >(ast))
    {
        co_await include_decl(as< ast2_module_declaration >(ast).declarations);
    }
    else if (typeis< ast2_namespace_declaration >(ast))
    {
        co_await include_decl(as< ast2_namespace_declaration >(ast).declarations);
    }

    else if (typeis< ast2_templex >(ast))
    {
        // TODO: probably redo this in a less hacky way.
        if (typeis< instanciation_reference >(input))
        {
            auto ast2 = co_await *c->lk_temploid_instanciation_ast(as< instanciation_reference >(input));
            assert(!typeis< ast2_templex >(ast2));
            goto start;
        }
        else
        {
            // This would be an error if done by a user, but we might check this during context lookup, so it needs to
            // not throw an error.

            // TODO: Have a parameter/option to throw an error for non-contextual lookups.

            co_return {};
        }
    }
    else if (typeis< functum >(ast))
    {
        // Functums cannot have member variables or global variables at the moment.
        co_return {};
    }
    else if (typeis< ast2_asm_procedure_declaration >(ast))
    {
        // Procedures don't contain anything.
        co_return {};
    }
    else
    {
        std::string typenam = to_string(input);
        std::string nametp = ast.type().name();
        rpnx::unimplemented();
    }

    output_type output;

    for (std::size_t i = 0; i < ast_declarations.size(); i++)
    {

        auto const& named_decl = ast_declarations[i];
        if (typeis< ast2_named_member >(named_decl))
        {

            auto const& member = as< ast2_named_member >(named_decl);
            auto const& name = member.name;
            auto const& decl = member.declaration;
            merge_entity(output.members[name], decl);
        }
        else if (typeis< ast2_named_global >(named_decl))
        {
            auto const& global = as< ast2_named_global >(named_decl);
            auto const& name = global.name;
            auto const& decl = global.declaration;
            merge_entity(output.globals[name], decl);
        }
        else
        {
            rpnx::unimplemented();
        }
    }
    co_return output;
}