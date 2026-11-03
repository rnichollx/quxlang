// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_BUILTIN_FUNCTIONS_HEADER_GUARD
#define QUXLANG_DATA_BUILTIN_FUNCTIONS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <rpnx/macros.hpp>
namespace quxlang
{

    /** Describes one generated or intrinsic overload and its execution contract. */
    struct builtin_function_info
    {
        temploid_ensig overload;

        // TODO: Convert this to optional instead of using void_type?
        type_symbol return_type;

        /** Selects ordinary invocation or a generated coroutine body. */
        function_execution_kind execution_kind = function_execution_kind::ordinary;
        /** Positional formals borrowed by INITIALIZE and constructed when the generated body starts. */
        std::set< std::size_t > owned_positional_parameters;
        /** Named formals borrowed by INITIALIZE and constructed when the generated body starts. */
        std::set< std::string > owned_named_parameters;

        RPNX_MEMBER_METADATA(builtin_function_info, overload, return_type, execution_kind, owned_positional_parameters, owned_named_parameters)
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_BUILTIN_FUNCTIONS_HEADER
