//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_TYPE_HEADER
#define RPNX_RYANSCRIPT1031_VM_TYPE_HEADER

#include <cstddef>
#include <boost/variant.hpp>
namespace rylang
{
    struct vm_type_int
    {
        std::size_t size;
        bool has_sign;
    };

    struct vm_type_pointer{};

    using vm_type = boost::variant<std::monostate, vm_type_int, vm_type_pointer>;
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_TYPE_HEADER
