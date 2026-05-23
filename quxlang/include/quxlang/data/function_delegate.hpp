// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_DELEGATE_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_DELEGATE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <rpnx/compare.hpp>

namespace quxlang
{
    struct function_delegate
    {
        type_symbol target;
        std::vector< expression > args;

        RPNX_MEMBER_METADATA(function_delegate, target, args);
    };
} // namespace quxlang

#endif // FUNCTION_DELEGATE_HEADER_GUARD
