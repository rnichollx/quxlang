// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ASM_PROCEDURE_FROM_SYMBOL_HEADER_GUARD
#define QUXLANG_QUERIES_ASM_PROCEDURE_FROM_SYMBOL_HEADER_GUARD

#include "quxlang/asm/asm.hpp"
#include <quxlang/data/type_symbol.hpp>

namespace quxlang
{
    struct asm_procedure_from_symbol_query
    {
        static constexpr auto query_id = "asm_procedure_from_symbol";
        using input_type = type_symbol;
        using output_type = asm_procedure;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ASM_PROCEDURE_FROM_SYMBOL_HEADER_GUARD
