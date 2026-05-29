// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_LLVM_BACKEND_HPP
#define QUXLANG_LLVM_BACKEND_HPP

#include <quxlang/llvm-backend-types.hpp>

namespace quxlang::llvm
{
    /**
     * Builds textual LLVM IR and bitcode for one VMIR2 compilation packet.
     */
    class llvm_backend
    {
    public:
        /**
         * Lowers one VMIR2 compilation packet into textual LLVM IR, bitcode, and object bytes.
         */
        auto compile(quxlang::llvm_backend::llvm_compilable_unit const& input) const -> quxlang::llvm_backend::llvm_compiled_unit;

        /**
         * Converts one machine-specific asm procedure into textual assembler plus object bytes.
         */
        auto assemble(
            quxlang::llvm_backend::llvm_compilation_target const& target,
            quxlang::asm_procedure const& procedure) const -> quxlang::llvm_backend::llvm_assembled_procedure;
    };
}

#endif //QUXLANG_LLVM_BACKEND_HPP
