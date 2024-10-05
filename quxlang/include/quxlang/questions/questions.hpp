// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_QUESTIONS_QUESTIONS_HEADER_GUARD
#define QUXLANG_QUESTIONS_QUESTIONS_HEADER_GUARD
#include "quxlang/asm/asm.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "rpnx/resolver2.hpp"

namespace quxlang::questions
{
    struct class_placement_info
    {
    };

    struct type_placement_info
    {
    };

    struct asm_procedure
    {
    };

}

namespace rpnx
{
    template <>
    struct question_traits< quxlang::questions::class_placement_info >
    {
        using input_type = quxlang::type_symbol;
        using output_type = quxlang::questions::class_placement_info;
    };

    template <>
    struct question_traits< quxlang::questions::type_placement_info >
    {
        using input_type = quxlang::type_symbol;
        using output_type = quxlang::questions::type_placement_info;
    };

    template <>
    struct question_traits< quxlang::questions::asm_procedure >
    {
        using input_type = quxlang::type_symbol;
        using output_type = quxlang::asm_procedure;
    };
}

#enddif //QUESTIONS_HPP
