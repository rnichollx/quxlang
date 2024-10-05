// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_TYPE_HEADER_GUARD
#define QUXLANG_DATA_VM_TYPE_HEADER_GUARD

#include <cstddef>
#include <boost/variant.hpp>
namespace quxlang
{
    struct vm_type_int
    {
        std::size_t size;
        bool has_sign;
    };

    struct vm_type_pointer{};

    using vm_type = boost::variant<std::monostate, vm_type_int, vm_type_pointer>;
} // namespace quxlang

#endif // QUXLANG_VM_TYPE_HEADER_GUARD
