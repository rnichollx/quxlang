// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_OPERATORS_HEADER_GUARD
#define QUXLANG_OPERATORS_HEADER_GUARD

#include <set>
#include <string>
namespace quxlang
{
    static std::set< std::string > const compare_operators = {"==", "!=", "<", ">", "<=", ">="};

    static std::set< std::string > const bitwise_operators = {"&&.", "||.", "^^.", "!^.", "!&.", "!|.", "?>.", "<?."};

    static std::set< std::string > const logic_operators = {"&&", "||", "^^", "!^", "!&", "!|", "?>", "<?"};

    static std::string const bracket_operator = "[]";

    static std::set< std::string > const base_operators = {":=", ":<"};
    static std::set< std::string > const arithmetic_operators = {"+", "-", "*", "/", "%"};

    static std::set< std::string > const pointer_arithmetic_operators = {"+", "-"};

    static std::set<std::string> const incdec_operators = {"++", "--"};



    static std::string const rightarrow_operator = "->";
    static std::string const leftarrow_operator = "<-";

    static std::string const doublerightarrow_operator = "=>>";
    static std::string const doubleleftarrow_operator = "<<=";


    static std::map< std::string, std::string > const builtin_funcs{
        {"INT(i)::.OPERATOR+(@THIS i, @OTHER i)", "i"}, {"INT(i)::.OPERATOR-(@THIS i, @OTHER i)", "i"}, {"INT(i)::.OPERATOR*(@THIS i, @OTHER i)", "i"}, {"INT(i)::.OPERATOR/(@THIS i, @OTHER i)", "i"}, {"INT(i)::.OPERATOR:=(@THIS OUT& i, @OTHER i)", "i"},

    };

} // namespace quxlang

#endif // QUXLANG_OPERATORS_HEADER_GUARD
