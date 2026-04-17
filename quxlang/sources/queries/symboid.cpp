// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/symboid_spec.hpp>
#include "quxlang/manipulators/merge_entity.hpp"
#include "quxlang/manipulators/typeutils.hpp"

namespace
{
    auto template_parameter_name(quxlang::type_symbol const& param) -> std::optional< std::string >
    {
        using namespace quxlang;

        if (typeis< auto_temploidic >(param))
        {
            auto const& name = as< auto_temploidic >(param).name;
            if (!name.empty())
            {
                return name;
            }
        }
        else if (typeis< type_temploidic >(param))
        {
            auto const& name = as< type_temploidic >(param).name;
            if (!name.empty())
            {
                return name;
            }
        }

        return std::nullopt;
    }

    auto template_parameter_name(quxlang::declared_parameter const& param) -> std::optional< std::string >
    {
        if (param.api_name.has_value())
        {
            return param.api_name;
        }

        return template_parameter_name(param.type);
    }

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


rpnx::querygraph::coroutine< quxlang::symboid_spec > quxlang::symboid_impl(type_symbol input)
{
    if (typeis< initialization_reference >(input))
    {
        auto const& init = as< initialization_reference >(input);
        auto inst = co_await rpnx::querygraph::request< instanciation_query >(init);
        if (!inst.has_value())
        {
            throw std::logic_error("symboid resolver received a non-canonical initialization_reference that could not be canonicalized");
        }
        co_return co_await rpnx::querygraph::request< symboid_query >(*inst);
    }

    if (typeis< instanciation_reference >(input))
    {
        auto const& inst = as< instanciation_reference >(input);
        auto temploid_kind = co_await rpnx::querygraph::request< symbol_type_query >(inst.temploid);

        if (temploid_kind == symbol_kind::template_)
        {
            auto const& sym = co_await rpnx::querygraph::request< symboid_query >(inst.temploid.templexoid);
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
                for (auto const& arg : tmpl.m_template_args.positional)
                {
                    auto canonical_arg = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                        .context = template_context,
                        .type = arg.type,
                    });

                    if (!canonical_arg.has_value())
                    {
                        valid = false;
                        break;
                    }

                    ensig.interface.positional.push_back(argif{.type = *canonical_arg});
                }

                for (auto const& [name, arg] : tmpl.m_template_args.named)
                {
                    auto canonical_arg = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                        .context = template_context,
                        .type = arg.type,
                    });

                    if (!canonical_arg.has_value())
                    {
                        valid = false;
                        break;
                    }

                    auto declared_name = template_parameter_name(arg).value_or(name);
                    ensig.interface.named[declared_name] = argif{.type = *canonical_arg};
                }

                if (valid && ensig == inst.temploid.which)
                {
                    co_return selected_template_decl_to_symboid(tmpl.m_declaroid);
                }
            }

            throw std::logic_error("Template declaration not found for instanciation");
        }
    }


    if (typeis< absolute_module_reference >(input))
    {
        auto const & module_ref = as< absolute_module_reference >(input);
        co_return co_await rpnx::querygraph::request< module_ast_query >(as< absolute_module_reference >(input).module_name);
    }

    auto declaroids = co_await rpnx::querygraph::request< declaroids_query >(input);

    ast2_symboid output;

    for (auto& decl : declaroids)
    {
        merge_entity(output, decl);
    }

    co_return output;
}
