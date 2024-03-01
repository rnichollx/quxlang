//
// Created by Ryan Nicholl on 9/21/23.
//
#include "rylang/manipulators/merge_entity.hpp"
#include <iostream>

void rylang::merge_entity(rylang::entity_ast& destination, const rylang::entity_ast& source)
{
    // std::cout << "Merge " << destination.to_string() << " and " << source.to_string() << std::endl;
    if (source.type() != destination.type())
    {
        throw std::runtime_error("Cannot merge entities of different types");
    }

    // if (destination.type() == entity_type::class_type)
    //{
    //     throw std::runtime_error("Cannot merge class entities");
    // }

    for (auto& x : source.m_sub_entities)
    {
        if (destination.m_sub_entities.count(x.first) == 0)
        {
            destination.m_sub_entities[x.first] = x.second;
        }
        else
        {
            merge_entity(destination.m_sub_entities[x.first].get(), x.second.get());
        }
    }

    if (source.type() == entity_type::function_type)
    {
        for (auto& x : std::get< functum_entity_ast >(source.m_specialization.get()).m_function_overloads)
        {
            std::get< functum_entity_ast >(destination.m_specialization.get()).m_function_overloads.push_back(x);
        }
    }
}
void rylang::merge_entity(ast2_map_entity& destination, ast2_declarable const& source)
{

    std::string kind = source.type().name();
    if (typeis< ast2_function_declaration >(source))
    {
        if (typeis< std::monostate >(destination))
        {
            destination = ast2_functum{};
        }
        else if (!typeis< ast2_functum >(destination))
        {
            throw std::runtime_error("Cannot merge function into non-function of the same name");
        }

        ast2_functum& destination_functum = boost::get< ast2_functum >(destination);
        destination_functum.functions.push_back(boost::get< ast2_function_declaration >(source));
    }
    else if (typeis< ast2_class_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw std::runtime_error("Cannot merge class into non-class of the same name");
        }

        destination = boost::get< ast2_class_declaration >(source);
    }
    else if (typeis< ast2_template_declaration >(source))
    {
        if (typeis< std::monostate >(destination))
        {
            destination = ast2_templex{};
        }
        if (!typeis< ast2_templex >(destination))
        {
            throw std::runtime_error("Cannot merge template into non-template of the same name");
        }

        ast2_templex& destination_templex = boost::get< ast2_templex >(destination);

        destination_templex.templates.push_back(boost::get< ast2_template_declaration >(source));
    }
    else if (typeis< ast2_namespace_declaration >(source))
    {
        if (typeis< std::monostate >(destination))
        {
            destination = ast2_namespace_declaration{};
        }
        if (!typeis< ast2_namespace_declaration >(destination))
        {
            throw std::runtime_error("Cannot merge namespace into non-namespace of the same name");
        }

        ast2_namespace_declaration& ns = as< ast2_namespace_declaration >(destination);

        for (auto& x : as< ast2_namespace_declaration >(source).globals)
        {
            ns.globals.push_back(x);
        }
    }
    else if (typeis< ast2_variable_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw std::runtime_error("Cannot merge variable into already existing entity");
        }

        destination = boost::get< ast2_variable_declaration >(source);
    }
    else if (typeis< ast2_asm_procedure_declaration >(source))
    {

        if (!typeis< std::monostate >(destination))
        {
            throw std::runtime_error("Cannot merge asm procedure into already existing entity");
        }

        destination = boost::get< ast2_asm_procedure_declaration >(source);
    }
    else
    {
        rpnx::unimplemented();
    }
}