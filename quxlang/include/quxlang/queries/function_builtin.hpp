// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTION_BUILTIN_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTION_BUILTIN_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <rpnx/macros.hpp>

/**
 * Classifies whether a selected function overload is builtin, and if so
 * which lowering path is required.
 */
RPNX_ENUM(quxlang, builtin_function_kind, std::uint8_t, not_builtin, builtin_intrinsic, builtin_generated_routine, builtin_special);

namespace quxlang
{
    /**
     * Returns the builtin lowering classification for a selected function
     * overload.
     */
    struct function_builtin_query
    {
        static constexpr auto query_id = "function_builtin";
        using input_type = temploid_reference;
        using output_type = builtin_function_kind;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTION_BUILTIN_HEADER_GUARD
