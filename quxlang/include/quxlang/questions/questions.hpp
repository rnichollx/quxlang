// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_QUESTIONS_QUESTIONS_HEADER_GUARD
#define QUXLANG_QUESTIONS_QUESTIONS_HEADER_GUARD
#include "quxlang/asm/asm.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "rpnx/resolver2.hpp"

namespace quxlang::questions
{
    struct class_placement_info_q
    {
    };

    struct type_placement_info_q
    {
    };

    struct asm_procedure_q
    {
    };

}

namespace rpnx
{
    template <>
    struct question_traits< quxlang::questions::class_placement_info_q >
    {
        using input_type = quxlang::type_symbol;
        using output_type = quxlang::questions::class_placement_info_q;
    };

    template <>
    struct question_traits< quxlang::questions::type_placement_info_q >
    {
        using input_type = quxlang::type_symbol;
        using output_type = quxlang::questions::type_placement_info_q;
    };

    template <>
    struct question_traits< quxlang::questions::asm_procedure_q >
    {
        using input_type = quxlang::type_symbol;
        using output_type = quxlang::asm_procedure;
    };
}

#endif //QUESTIONS_HPP
