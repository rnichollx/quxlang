// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_BUILTIN_FUNCTIONS_HEADER_GUARD
#define QUXLANG_DATA_BUILTIN_FUNCTIONS_HEADER_GUARD

#include "type_symbol.hpp"

#include <rpnx/metadata.hpp>
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
