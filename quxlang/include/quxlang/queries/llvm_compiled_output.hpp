// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_COMPILED_OUTPUT_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_COMPILED_OUTPUT_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

namespace quxlang
{
    /** One independently generated relocatable object and its compilation identity. */
    struct llvm_output_object
    {
        llvm_output_query_input identity;
        llvm_backend::llvm_preoptimized_unit preoptimized;
        llvm_backend::llvm_postoptimized_unit postoptimized;
        llvm_backend::llvm_post_codegen_unit post_codegen;

        RPNX_MEMBER_METADATA(llvm_output_object, identity, preoptimized, postoptimized, post_codegen);
    };

    /** Ordered native-link inputs for one configured output. */
    struct llvm_compiled_output
    {
        std::vector< llvm_output_object > objects;

        RPNX_MEMBER_METADATA(llvm_compiled_output, objects);
    };

    /// llvm_compiled_output_query compiles and assembles every LLVM component required by one configured output.
    struct llvm_compiled_output_query
    {
        static constexpr auto query_id = "llvm_compiled_output";
        using input_type = std::string;
        using output_type = llvm_compiled_output;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_COMPILED_OUTPUT_HEADER_GUARD
