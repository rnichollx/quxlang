// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/manipulators/merge_entity.hpp"
#include <iostream>


void quxlang::merge_entity(ast2_symboid& destination, ast2_declarable const& source)
{

    std::string kind = source.type().name();
    if (typeis< ast2_function_declaration >(source))
    {

        auto const& func = as< ast2_function_declaration >(source);

        if (typeis< std::monostate >(destination))
        {
            destination = functum{};
        }

        else if (!typeis< functum >(destination))
        {
            throw std::logic_error("Cannot merge function into non-function of the same name");
        }

        functum & destination_functum = as< functum >(destination);

        temploid_ensig ol;

        // TODO: Consider exporting the extraction of call_type
        //  from call_paramters to a function
        for (ast2_function_parameter const& param : func.header.call_parameters)
        {
            if (param.api_name.has_value())
            {
                rpnx::unimplemented();
            }
            ol.interface.positional.push_back(param.type);
        }

       // auto it = destination_functum.functions.find(ol);
       // if (it != destination_functum.functions.end())
       // {
       //     throw std::logic_error("Functum already declared with the same overload");
       // }

        destination_functum.functions.push_back(func);
    }
    else if (typeis< ast2_class_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw std::logic_error("Cannot merge class into non-class of the same name");
        }

        destination = as< ast2_class_declaration >(source);
    }
    else if (typeis< ast2_template_declaration >(source))
    {
        if (typeis< std::monostate >(destination))
        {
            destination = ast2_templex{};
        }
        if (!typeis< ast2_templex >(destination))
        {
            throw std::logic_error("Cannot merge template into non-template of the same name");
        }

        ast2_templex& destination_templex = as< ast2_templex >(destination);

        destination_templex.templates.push_back(as< ast2_template_declaration >(source));
    }
    else if (typeis< ast2_namespace_declaration >(source))
    {
        if (typeis< std::monostate >(destination))
        {
            destination = ast2_namespace_declaration{};
        }
        if (!typeis< ast2_namespace_declaration >(destination))
        {
            throw std::logic_error("Cannot merge namespace into non-namespace of the same name");
        }

        ast2_namespace_declaration& ns = as< ast2_namespace_declaration >(destination);

        for (auto& x : as< ast2_namespace_declaration >(source).declarations)
        {
            ns.declarations.push_back(x);
        }
    }
    else if (typeis< ast2_variable_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw std::logic_error("Cannot merge variable into already existing entity");
        }

        destination = as< ast2_variable_declaration >(source);
    }
    else if (typeis< ast2_asm_procedure_declaration >(source))
    {

        if (!typeis< std::monostate >(destination))
        {
            throw std::logic_error("Cannot merge asm procedure into already existing entity");
        }

        destination = as< ast2_asm_procedure_declaration >(source);
    }
    else
    {
        rpnx::unimplemented();
    }
}