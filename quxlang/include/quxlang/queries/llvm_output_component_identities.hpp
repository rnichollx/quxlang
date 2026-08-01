// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_OUTPUT_COMPONENT_IDENTITIES_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_OUTPUT_COMPONENT_IDENTITIES_HEADER_GUARD

#include <quxlang/queries/output_llvm_input.hpp>

#include <string>
#include <vector>

namespace quxlang
{
    /** Returns every independently compiled component identity for one output in linker order. */
    struct llvm_output_component_identities_query
    {
        static constexpr auto query_id = "llvm_output_component_identities";
        using input_type = std::string;
        using output_type = std::vector< llvm_output_query_input >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_OUTPUT_COMPONENT_IDENTITIES_HEADER_GUARD
