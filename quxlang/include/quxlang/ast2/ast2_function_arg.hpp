//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef AST2_FUNCTION_ARG_HPP
#define AST2_FUNCTION_ARG_HPP
#include <quxlang/data/type_symbol.hpp>

namespace quxlang
{
    struct ast2_function_arg
    {
        std::string name;
        std::optional< std::string > api_name;
        type_symbol type;

        RPNX_MEMBER_METADATA(ast2_function_arg, name, api_name, type)
    };
}


#endif //AST2_FUNCTION_ARG_HPP