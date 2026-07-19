// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_LLVM_BACKEND_INTERNAL_HEADER_GUARD
#define QUXLANG_SOURCES_LLVM_BACKEND_INTERNAL_HEADER_GUARD

#include <quxlang/llvm-backend.hpp>

#include <llvm/IR/IRBuilder.h>

namespace quxlang::llvm_backend::detail
{
    /** LLVM instruction builder used by the Quxlang module generator. */
    using ir_builder_t = llvm::IRBuilder< llvm::ConstantFolder, llvm::IRBuilderCallbackInserter >;

    /** Rejects enum metadata without a canonical fixed-width representation. */
    void require_canonical_enum_value(quxlang::enum_info const& info, std::vector< std::byte > const& value);

    /** Describes one source-level ABI parameter. */
    struct abi_parameter;
    /** Describes one VMIR routine parameter at the ABI boundary. */
    struct routine_abi_parameter;
    /** Describes the LLVM ABI projection of one callable. */
    struct callable_abi;
    /** Describes one constant aggregate storage segment. */
    struct constant_storage_segment;
    /** Describes one resolved antestatal object. */
    struct resolved_antestatal_object;
    /** Tracks the state of one local LLVM slot. */
    struct local_slot_state;
    /** Tracks LLVM generation state for one function. */
    struct function_codegen_state;
    /** Generates one LLVM module from a Quxlang compilation unit. */
    class llvm_module_codegen;
} // namespace quxlang::llvm_backend::detail

#endif // QUXLANG_SOURCES_LLVM_BACKEND_INTERNAL_HEADER_GUARD
