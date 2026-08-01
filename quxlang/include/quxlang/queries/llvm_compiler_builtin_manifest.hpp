// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_COMPILER_BUILTIN_MANIFEST_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_COMPILER_BUILTIN_MANIFEST_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <map>
#include <string>

namespace quxlang
{
    /** Returns the compiler-defined object types referenced by every component of one output. */
    struct llvm_compiler_builtin_manifest_query
    {
        static constexpr auto query_id = "llvm_compiler_builtin_manifest";
        using input_type = std::string;
        using output_type = std::map< type_symbol, type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_COMPILER_BUILTIN_MANIFEST_HEADER_GUARD
