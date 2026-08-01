// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_LLVM_BACKEND_HPP
#define QUXLANG_LLVM_BACKEND_HPP

#include <quxlang/llvm-backend-types.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace quxlang::llvm_backend
{
    /**
     * Returns the LLVM target configuration for one program stepping.
     *
     * Debug compilation remains generic. Release compilation translates the
     * stepping's required and rejected CPU attributes into LLVM target data.
     */
    auto llvm_compilation_target_for_stepping(
        machine_target_info const& machine,
        optimization_level optimization,
        cpu_stepping_configuration const& stepping) -> llvm_compilation_target;

    /**
     * Builds textual LLVM IR and bitcode for one VMIR2 compilation packet.
     */
    class llvm_backend
    {
    public:
        /** Lowers one VMIR2 compilation packet to verified pre-optimization LLVM. */
        auto preoptimize(
            quxlang::llvm_backend::llvm_compilable_unit const& input) const -> quxlang::llvm_backend::llvm_preoptimized_unit;

        /** Runs the configured optimization stage without performing machine code generation. */
        auto postoptimize(
            quxlang::llvm_backend::llvm_preoptimized_unit const& input) const -> quxlang::llvm_backend::llvm_postoptimized_unit;

        /** Generates one independently linkable native object from post-optimization LLVM. */
        auto post_codegen(
            quxlang::llvm_backend::llvm_postoptimized_unit const& input) const -> quxlang::llvm_backend::llvm_post_codegen_unit;

        /**
         * Converts one machine-specific asm procedure into textual assembler plus object bytes.
         */
        auto assemble(
            quxlang::llvm_backend::llvm_compilation_target const& target,
            quxlang::asm_procedure const& procedure) const -> quxlang::llvm_backend::llvm_assembled_procedure;
    };
}

#endif //QUXLANG_LLVM_BACKEND_HPP
