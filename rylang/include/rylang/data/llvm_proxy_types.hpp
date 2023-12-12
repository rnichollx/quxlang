//
// Created by Ryan Nicholl on 10/28/23.
//

#ifndef RYLANG_LLVM_PROXY_TYPES_HEADER_GUARD
#define RYLANG_LLVM_PROXY_TYPES_HEADER_GUARD

#include <boost/variant.hpp>

namespace rylang
{
    struct llvm_proxy_type_int
    {
        int bits = 0;
        bool has_sign = false;
    };

    struct llvm_proxy_type_pointer
    {};

    using llvm_proxy_type = boost::variant< llvm_proxy_type_pointer, llvm_proxy_type_int >;
} // namespace rylang

#endif // RYLANG_LLVM_PROXY_TYPES_HEADER_GUARD
