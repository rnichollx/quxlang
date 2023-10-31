//
// Created by Ryan Nicholl on 9/21/23.
//
#include "rylang/manipulators/merge_entity.hpp"
#include <iostream>

void rylang::merge_entity(rylang::entity_ast& destination, const rylang::entity_ast& source)
{
    //std::cout << "Merge " << destination.to_string() << " and " << source.to_string() << std::endl;
    if (source.type() != destination.type())
    {
        throw std::runtime_error("Cannot merge entities of different types");
    }

    //if (destination.type() == entity_type::class_type)
    //{
    //    throw std::runtime_error("Cannot merge class entities");
    //}

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