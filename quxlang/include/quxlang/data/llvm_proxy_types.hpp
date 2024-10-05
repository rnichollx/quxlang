// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_LLVM_PROXY_TYPES_HEADER_GUARD
#define QUXLANG_DATA_LLVM_PROXY_TYPES_HEADER_GUARD

#include <boost/variant.hpp>

namespace quxlang
{
    struct llvm_proxy_type_int
    {
        int bits = 0;
        bool has_sign = false;
    };

    struct llvm_proxy_type_pointer
    {};

    using llvm_proxy_type = boost::variant< llvm_proxy_type_pointer, llvm_proxy_type_int >;
} // namespace quxlang

#endif // QUXLANG_LLVM_PROXY_TYPES_HEADER_GUARD
