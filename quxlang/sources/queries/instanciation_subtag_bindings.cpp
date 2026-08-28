// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/instanciation_subtag_bindings_spec.hpp>

#include <quxlang/ast2/ast2_entity.hpp>

rpnx::querygraph::coroutine< quxlang::instanciation_subtag_bindings_spec > quxlang::instanciation_subtag_bindings_impl(instanciation_reference input)
{
    std::map< std::string, parameter_instantiation > output;
    symbol_kind const instanciated_kind = co_await rpnx::querygraph::request< symbol_type_query >(input.temploid);
    if (instanciated_kind == symbol_kind::template_)
    {
        output = input.params.named;

        auto selected_parameters = [&]() -> rpnx::querygraph::coroutine< instanciation_subtag_bindings_spec >::cosubroutine< declared_parameters const* >
        {
            if (co_await rpnx::querygraph::request< template_builtin_query >(input.temploid))
            {
                std::vector< builtin_template_info > const& builtin_templates = co_await rpnx::querygraph::request< templex_builtins_query >(input.temploid.templexoid);
                std::uint64_t const template_index = input.temploid.overload_id.value_or(0);
                if ((!input.temploid.overload_id.has_value() && builtin_templates.size() != 1) || template_index >= builtin_templates.size())
                {
                    throw compiler_bug("Template argument binding requires one selected builtin template");
                }
                co_return &builtin_templates.at(static_cast< std::vector< builtin_template_info >::size_type >(template_index)).template_args;
            }

            ast2_symboid const& symbol = co_await rpnx::querygraph::request< symboid_query >(input.temploid.templexoid);
            if (!typeis< ast2_templex >(symbol))
            {
                throw compiler_bug("Template argument binding requires a templex declaration");
            }
            ast2_templex const& templex = as< ast2_templex >(symbol);
            std::uint64_t const template_index = input.temploid.overload_id.value_or(0);
            if ((!input.temploid.overload_id.has_value() && templex.templates.size() != 1) || template_index >= templex.templates.size())
            {
                throw compiler_bug("Template argument binding requires one selected template declaration");
            }
            co_return &templex.templates.at(static_cast< std::vector< ast2_template_declaration >::size_type >(template_index)).m_template_args;
        };

        declared_parameters const& parameters = *(co_await selected_parameters());
        if (input.params.positional.size() != parameters.positional.size())
        {
            throw compiler_bug("Template positional argument count does not match its declaration");
        }
        for (std::size_t index = 0; index < input.params.positional.size(); index++)
        {
            declared_parameter const& parameter = parameters.positional.at(index);
            if (!parameter.name.has_value())
            {
                throw compiler_bug("A positional template argument is missing its declaration binding name");
            }
            output[*parameter.name] = input.params.positional.at(index);
        }
    }

    temploid_instanciation_parameter_set const tempars = co_await rpnx::querygraph::request< instanciation_tempar_map_query >(input);
    for (std::map< std::string, type_symbol >::value_type const& binding : tempars.parameter_map)
    {
        std::string const& name = binding.first;
        type_symbol const& type = binding.second;
        if (!output.contains(name))
        {
            output[name] = make_type_instantiation(type);
        }
    }

    co_return output;
}
