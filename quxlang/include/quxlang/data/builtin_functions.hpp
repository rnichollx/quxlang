//
// Created by Ryan Nicholl on 4/26/24.
//

#ifndef RPNX_QUXLANG_BUILTIN_FUNCTIONS_HEADER
#define RPNX_QUXLANG_BUILTIN_FUNCTIONS_HEADER

#include "type_symbol.hpp"

#include <rpnx/metadata.hpp>
namespace quxlang
{

    struct primitive_function_info
    {
        function_overload overload;

        // TODO: Convert this to optional instead of using void_type?
        type_symbol return_type;

        RPNX_MEMBER_METADATA(primitive_function_info, overload, return_type)
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_BUILTIN_FUNCTIONS_HEADER
