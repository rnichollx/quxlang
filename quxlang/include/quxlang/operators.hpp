//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef QUXLANG_OPERATORS_HEADER_GUARD
#define QUXLANG_OPERATORS_HEADER_GUARD

#include <set>
#include <string>
namespace quxlang
{
    static std::set< std::string > const bool_operators = {"==", "!=", "<", ">", "<=", ">="};

    static std::set< std::string > const assignment_operators = {":=", ":<"};
} // namespace quxlang

#endif // QUXLANG_OPERATORS_HEADER_GUARD
