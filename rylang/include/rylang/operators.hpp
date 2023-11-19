//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef RPNX_RYANSCRIPT1031_OPERATORS_HEADER
#define RPNX_RYANSCRIPT1031_OPERATORS_HEADER

#include <set>
#include <string>
namespace rylang
{
    static std::set< std::string > const bool_operators = {"==", "!=", "<", ">", "<=", ">="};

    static std::set< std::string > const assignment_operators = {":=", ":<"};
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_OPERATORS_HEADER
