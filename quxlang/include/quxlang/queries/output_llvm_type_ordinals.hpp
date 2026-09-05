// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_OUTPUT_LLVM_TYPE_ORDINALS_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LLVM_TYPE_ORDINALS_HEADER_GUARD
#include <quxlang/llvm-backend-types.hpp>
namespace quxlang
{
    /** Assigns one shared type-index namespace to all units of an output. */
    struct output_llvm_type_ordinals_query
    {
        static constexpr auto query_id = "output_llvm_type_ordinals";
        using input_type = std::string;
        using output_type = rpnx::cow< std::map< type_symbol, std::uint64_t > >;
    };
} // namespace quxlang
#endif
