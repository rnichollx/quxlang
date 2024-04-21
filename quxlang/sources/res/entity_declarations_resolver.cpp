//
// Created by Ryan Nicholl on 4/20/24.
//

#include <quxlang/compiler.hpp>
#include <quxlang/res/declaroids_resolver.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(declaroids)
{

    std::vector< ast2_declarable > declarations;

    std::string inputname = to_string(input);

    if (typeis< module_reference >(input))
    {
        throw std::logic_error("Cannot have declarations of a module");
    }

    if (typeis< instanciation_reference >(input))
    {
        // TODO: Maybe we allow this for templates?
        throw std::logic_error("Instancations are not declarables");
    }

    bool is_member = false;
    std::string subname;

    if (typeis< subentity_reference >(input))
    {
        subname = as< subentity_reference >(input).subentity_name;
    }
    else if (typeis< subdotentity_reference >(input))
    {
        is_member = true;
        subname = as< subdotentity_reference >(input).subdotentity_name;
    }
    else
    {
        co_return {};
    }

    type_symbol parent_addr = qualified_parent(input).value();

    ast2_symboid sym = co_await QUX_CO_DEP(symboid, (parent_addr));

start:

    auto include_decl = [&](std::vector< ast2_top_declaration > const& decls) -> rpnx::general_coroutine< compiler, void >
    {
        for (auto const& decl : decls)
        {
            if (typeis< ast2_include_if >(decl))
            {
                auto& inc = as< ast2_include_if >(decl);
                expr_interp_input i_input{.context = input, .expression = inc.condition};

                bool should_include = true; // co_await *c->lk_interpret_bool(i_input);
                // TODO: Implement interpret_bool and uncomment the above line.

                if (!should_include)
                {
                    continue;
                }

                if (typeis< ast2_named_member >(inc.declaration))
                {
                    if (as< ast2_named_member >(inc.declaration).name != subname || !is_member)
                    {
                        continue;
                    }
                    else
                    {
                        declarations.push_back(as< ast2_named_member >(inc.declaration).declaration);
                    }
                }
                else if (typeis< ast2_named_global >(inc.declaration))
                {
                    if (as< ast2_named_global >(inc.declaration).name != subname || is_member)
                    {
                        continue;
                    }
                    else
                    {
                        declarations.push_back(as< ast2_named_global >(inc.declaration).declaration);
                    }
                }
                else
                {
                    rpnx::unimplemented();
                }


            }
            else if (typeis< ast2_named_declaration >(decl))
            {

                auto decl2 = as< ast2_named_declaration >(decl);
                if (typeis< ast2_named_member >(decl2))
                {
                    if (as< ast2_named_member >(decl2).name != subname || !is_member)
                    {
                        continue;
                    }
                }
                else if (typeis< ast2_named_global >(decl2))
                {
                    if (as< ast2_named_global >(decl2).name != subname || is_member)
                    {
                        continue;
                    }
                }
                else
                {
                    rpnx::unimplemented();
                }
            }
        }
        co_return;
    };

    if (typeis< ast2_class_declaration >(sym))
    {
        co_await include_decl(as< ast2_class_declaration >(sym).declarations);
    }
    else if (typeis< ast2_module_declaration >(sym))
    {
        co_await include_decl(as< ast2_module_declaration >(sym).declarations);
    }
    else if (typeis< ast2_namespace_declaration >(sym))
    {
        co_await include_decl(as< ast2_namespace_declaration >(sym).declarations);
    }

    else if (typeis< ast2_templex >(sym))
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
    else if (typeis< functum >(sym))
    {
        // Functums cannot have member variables or global variables at the moment.
        co_return {};
    }
    else if (typeis< ast2_asm_procedure_declaration >(sym))
    {
        // Procedures don't contain anything.
        co_return {};
    }
    else
    {
        std::string typenam = to_string(input);
        std::string nametp = sym.type().name();
        rpnx::unimplemented();
    }

    co_return declarations;
}