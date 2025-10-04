// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_TEMPLOID_INSTANCIATION_PARAMETER_SET_HEADER_GUARD
#define QUXLANG_DATA_TEMPLOID_INSTANCIATION_PARAMETER_SET_HEADER_GUARD
#include <map>
#include <quxlang/data/type_symbol.hpp>

namespace quxlang
{
    struct temploid_instanciation_parameter_set
    {
        std::map< std::string, type_symbol > parameter_map;

        RPNX_MEMBER_METADATA(temploid_instanciation_parameter_set, parameter_map);
    };
}

template <>
struct rpnx::resolver_traits<quxlang::temploid_instanciation_parameter_set>
{
    static std::string stringify(quxlang::temploid_instanciation_parameter_set const& v)
    {
        std::string result = "{";
        bool first = true;
        for (auto& [k, v] : v.parameter_map)
        {
            if (!first)
            {
                result += ", ";
            }
            first = false;
            result += k + ": " + quxlang::to_string(v);
        }
        result += "}";
        return result;
    }
};

#endif //TEMPLATE_INSTANCIATION_PARAMETER_SET_HPP
