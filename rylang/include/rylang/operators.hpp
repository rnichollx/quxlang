//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef RYLANG_OPERATORS_HEADER_GUARD
#define RYLANG_OPERATORS_HEADER_GUARD

#include <set>
#include <string>
namespace rylang
{
    static std::set< std::string > const bool_operators = {"==", "!=", "<", ">", "<=", ">="};

    static std::set< std::string > const assignment_operators = {":=", ":<"};
} // namespace rylang

#endif // RYLANG_OPERATORS_HEADER_GUARD
