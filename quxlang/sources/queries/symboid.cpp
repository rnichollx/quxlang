// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/symboid_spec.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/merge_entity.hpp"
#include "quxlang/manipulators/typeutils.hpp"

#include <optional>
#include <string>

rpnx::querygraph::coroutine< quxlang::symboid_spec > quxlang::symboid_impl(type_symbol input)
{
    auto selected_template_decl_to_symboid = [](declaroid const& decl) -> ast2_symboid
    {
        if (typeis< ast2_class_declaration >(decl))
        {
            return as< ast2_class_declaration >(decl);
        }
        else if (typeis< ast2_interface_declaration >(decl))
        {
            return as< ast2_interface_declaration >(decl);
        }
        else if (typeis< ast2_implementation_declaration >(decl))
        {
            return as< ast2_implementation_declaration >(decl);
        }
        else if (typeis< ast2_enum_declaration >(decl))
        {
            return as< ast2_enum_declaration >(decl);
        }
        else if (typeis< ast2_flagset_declaration >(decl))
        {
            return as< ast2_flagset_declaration >(decl);
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
        else if (typeis< ast2_unit_test >(decl))
        {
            return as< ast2_unit_test >(decl);
        }
        else
        {
            throw compiler_bug("Unsupported selected template declaroid");
        }
    };

    auto template_parameter_name = [](declared_parameter const& param) -> std::optional< std::string >
    {
        if (param.name.has_value())
        {
            return param.name;
        }
        if (param.api_name.has_value())
        {
            return param.api_name;
        }

        auto const& param_type = param.type;
        if (typeis< auto_temploidic >(param_type))
        {
            auto const& name = as< auto_temploidic >(param_type).name;
            if (!name.empty())
            {
                return name;
            }
        }
        else if (typeis< decay_temploidic >(param_type))
        {
            auto const& name = as< decay_temploidic >(param_type).name;
            if (!name.empty())
            {
                return name;
            }
        }
        else if (typeis< type_temploidic >(param_type))
        {
            auto const& name = as< type_temploidic >(param_type).name;
            if (!name.empty())
            {
                return name;
            }
        }

        return std::nullopt;
    };

    if (typeis< initialization_reference >(input))
    {
        auto const& init = as< initialization_reference >(input);
        auto inst = co_await rpnx::querygraph::request< instanciation_query >(init);
        if (!inst.has_value())
        {
            throw quxlang::compiler_bug("symboid resolver received a non-canonical initialization_reference that could not be canonicalized");
        }
        co_return co_await rpnx::querygraph::request< symboid_query >(*inst);
    }

    if (typeis< instanciation_reference >(input))
    {
        auto const& inst = as< instanciation_reference >(input);
        auto temploid_kind = co_await rpnx::querygraph::request< symbol_type_query >(inst.temploid);

        if (temploid_kind == symbol_kind::template_)
        {
            if (co_await rpnx::querygraph::request< template_builtin_query >(inst.temploid))
            {
                if (auto atomic_value_type = atomic_type_argument(input); atomic_value_type.has_value())
                {
                    if (!is_valid_atomic_storage_type(*atomic_value_type))
                    {
                        co_return std::monostate{};
                    }

                    ast2_class_declaration builtin_atomic_class;
                    builtin_atomic_class.class_keywords.insert(keywords::no_implicit_assignment);
                    builtin_atomic_class.class_keywords.insert(keywords::not_copyable);
                    co_return builtin_atomic_class;
                }

                functum builtin_functum;
                co_return builtin_functum;
            }

            auto const& sym = co_await rpnx::querygraph::request< symboid_query >(inst.temploid.templexoid);
            if (!typeis< ast2_templex >(sym))
            {
                throw compiler_bug("template instanciation parent did not resolve to ast2_templex");
            }

            auto const& templex = as< ast2_templex >(sym);
            std::uint64_t template_index = inst.temploid.overload_id.value_or(0);
            if (!inst.temploid.overload_id.has_value() && templex.templates.size() != 1)
            {
                throw quxlang::compiler_bug("Template instanciation did not resolve to a unique template declaration");
            }

            if (template_index >= templex.templates.size())
            {
                throw quxlang::compiler_bug("Template overload id is out of range for instanciation symboid lookup");
            }

            co_return selected_template_decl_to_symboid(templex.templates.at(static_cast< std::vector< ast2_template_declaration >::size_type >(template_index)).m_declaroid);
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
