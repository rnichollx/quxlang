// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_EXPRESSION_STRINGIFIER_HEADER_GUARD
#define QUXLANG_MANIPULATORS_EXPRESSION_STRINGIFIER_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include "quxlang/data/expression_numeric_literal.hpp"
#include "quxlang/data/numeric_literal.hpp"
#include <string>

namespace quxlang
{

    std::string to_string(expression const& expr);



    std::string to_string(expression const& expr);
} // namespace quxlang

#endif // QUXLANG_EXPRESSION_STRINGIFIER_HEADER_GUARD
