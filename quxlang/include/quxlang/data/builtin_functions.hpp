// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_BUILTIN_FUNCTIONS_HEADER_GUARD
#define QUXLANG_DATA_BUILTIN_FUNCTIONS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <rpnx/macros.hpp>
namespace quxlang
{

    struct builtin_function_info
    {
        temploid_ensig overload;

        // TODO: Convert this to optional instead of using void_type?
        type_symbol return_type;

        RPNX_MEMBER_METADATA(builtin_function_info, overload, return_type)
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_BUILTIN_FUNCTIONS_HEADER
