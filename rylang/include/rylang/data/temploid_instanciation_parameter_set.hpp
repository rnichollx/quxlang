//
// Created by Ryan Nicholl on 12/18/23.
//

#ifndef TEMPLATE_INSTANCIATION_PARAMETER_SET_HPP
#define TEMPLATE_INSTANCIATION_PARAMETER_SET_HPP
#include <map>
#include <rylang/data/qualified_symbol_reference.hpp>

namespace rylang
{
    struct temploid_instanciation_parameter_set
    {
        std::map< std::string, type_symbol > parameter_map;
    };
}

#endif //TEMPLATE_INSTANCIATION_PARAMETER_SET_HPP
