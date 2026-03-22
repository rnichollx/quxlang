// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/manipulators/merge_entity.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/res/symboid_resolver.hpp>

namespace
{
    auto selected_template_decl_to_symboid(quxlang::declaroid const& decl) -> quxlang::ast2_symboid
    {
        using namespace quxlang;

        if (typeis< ast2_class_declaration >(decl))
        {
            return as< ast2_class_declaration >(decl);
        }
        else if (typeis< ast2_function_declaration >(decl))
        {
            functum output;
            output.functions.push_back(as< ast2_function_declaration >(decl));
            return output;
        }
        else if (typeis< ast2_variable_declaration >(decl))
        {
            return as< ast2_variable_declaration >(decl);
        }
        else if (typeis< ast2_namespace_declaration >(decl))
        {
            return as< ast2_namespace_declaration >(decl);
        }
        else if (typeis< ast2_extern >(decl))
        {
            return as< ast2_extern >(decl);
        }
        else if (typeis< ast2_asm_procedure_declaration >(decl))
        {
            return as< ast2_asm_procedure_declaration >(decl);
        }
        else if (typeis< ast2_static_test >(decl))
        {
            return as< ast2_static_test >(decl);
        }
        else
        {
            throw compiler_bug("Unsupported selected template declaroid");
        }
    }
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(symboid)
{
    if (typeis< initialization_reference >(input_val))
    {
        auto const& init = as< initialization_reference >(input_val);
        auto inst = co_await QUX_CO_DEP(instanciation, (init));
        if (!inst.has_value())
        {
            throw std::logic_error("symboid resolver received a non-canonical initialization_reference that could not be canonicalized");
        }
        co_return co_await QUX_CO_DEP(symboid, (*inst));
    }

    if (typeis< instanciation_reference >(input_val))
    {
        auto const& inst = as< instanciation_reference >(input_val);
        auto temploid_kind = co_await QUX_CO_DEP(symbol_type, (inst.temploid));

        if (temploid_kind == symbol_kind::template_)
        {
            auto const& sym = co_await QUX_CO_DEP(symboid, (inst.temploid.templexoid));
            if (!typeis< ast2_templex >(sym))
            {
                throw compiler_bug("template instanciation parent did not resolve to ast2_templex");
            }

            auto const& templex = as< ast2_templex >(sym);
            auto template_context = type_parent(inst.temploid.templexoid).value_or(context_reference{});

            for (auto const& tmpl : templex.templates)
            {
                temploid_ensig ensig;
                ensig.priority = tmpl.priority;

                bool valid = true;
                for (auto const& arg : tmpl.m_template_args)
                {
                    auto canonical_arg = co_await QUX_CO_DEP(lookup, (contextual_type_reference{
                        .context = template_context,
                        .type = arg,
                    }));

                    if (!canonical_arg.has_value())
                    {
                        valid = false;
                        break;
                    }

                    ensig.interface.positional.push_back(argif{.type = *canonical_arg});
                }

                if (valid && ensig == inst.temploid.which)
                {
                    co_return selected_template_decl_to_symboid(tmpl.m_declaroid);
                }
            }

            throw std::logic_error("Template declaration not found for instanciation");
        }
    }


    if (typeis< absolute_module_reference >(input_val))
    {
        auto const & module_ref = as< absolute_module_reference >(input_val);
        co_return co_await QUX_CO_DEP(module_ast, (as< absolute_module_reference >(input_val).module_name));
    }

    auto declaroids = co_await QUX_CO_DEP(declaroids, (input_val));

    ast2_symboid output;

    for (auto& decl : declaroids)
    {
        merge_entity(output, decl);
    }

    co_return output;
}
