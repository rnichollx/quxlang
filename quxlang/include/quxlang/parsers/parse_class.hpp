//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_CLASS_HPP
#define PARSE_CLASS_HPP
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/parsers/try_parse_class.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_class_declaration > try_parse_class(It& pos, It end);

    template <typename It>
    ast2_class_declaration parse_class(It & pos, It end)
    {
        auto result = try_parse_class(pos, end);
        if (!result)
        {
            throw std::logic_error("Expected class");
        }
        return result.value();
    }
}

#endif //PARSE_CLASS_HPP
