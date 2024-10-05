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

    static std::set<std::string> const bitwise_operators = {
        "&&.", "||.", "^^.", "!^.", "!&.", "!|.", "?>.", "<?."
    };

    static std::set<std::string> const logic_operators = {
        "&&", "||", "^^", "!^", "!&", "!|", "?>", "<?"
    };

    static std::set< std::string > const assignment_operators = {":=", ":<"};
    static std::set<std::string > const arithmetic_operators = {"+", "-", "*", "/", "%"};

    static std::map<std::string, std::string> const builtin_funcs
    {
       {"INT(i)::.OPREATOR+(@THIS i, @OTHER i)", "i"},
        {"INT(i)::.OPREATOR-(@THIS i, @OTHER i)", "i"},
{"INT(i)::.OPERATOR*(@THIS i, @OTHER i)", "i"},
{"INT(i)::.OPERATOR/(@THIS i, @OTHER i)", "i"},
{"INT(i)::.OPERATOR:=(@THIS OUT& i, @OTHER i)", "i"},

    };



} // namespace quxlang

#endif // QUXLANG_OPERATORS_HEADER_GUARD
