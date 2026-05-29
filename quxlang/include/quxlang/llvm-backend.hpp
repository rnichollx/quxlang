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
         * Lowers one VMIR2 compilation packet into textual LLVM IR plus bitcode bytes.
         */
        auto compile(quxlang::llvm_backend::llvm_compilable_unit const& input) const -> quxlang::llvm_backend::llvm_compiled_unit;
    };
}

#endif //QUXLANG_LLVM_BACKEND_HPP
