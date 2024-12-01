// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_TO_PRETTY_STRING_HEADER_GUARD
#define QUXLANG_TO_PRETTY_STRING_HEADER_GUARD

#include "data/expression.hpp"
#include "data/function_block.hpp"
#include "data/type_symbol.hpp"
#include "data/vm_executable_unit.hpp"
#include "data/vm_expression.hpp"
#include <string>

namespace quxlang
{
    // the same idea as to_string, except this one provides an indented version with newlines
    // Rules:
    // 1. Everything returns with a newline at the end
    // 2. Nothing provides initial indentation before the first line
    // 3. Temporarily increase indentation by 1 before adding a sub member
    // 4. should return the result string (if doesn't have this, it's a bug)






} // namespace quxlang

#endif // TO_PRETTY_STRING_HEADER_GUARD
