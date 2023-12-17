//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_CLASS_HPP
#define PARSE_CLASS_HPP
#include <rylang/ast2/ast2_type_map.hpp>

namespace rylang::parsers
{
    template <typename It>
    ast2_class_declaration parse_class(It & pos, It end)
    {
        auto result = try_parse_class(pos, end);
        if (!result)
        {
            throw std::runtime_error("Expected class");
        }
        return result.value();
    }
}

#endif //PARSE_CLASS_HPP
