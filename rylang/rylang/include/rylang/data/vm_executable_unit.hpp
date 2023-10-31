//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER

#include <boost/variant.hpp>

#include "vm_allocate_storage.hpp"

namespace rylang
{
    struct vm_block;
    struct vm_allocate_storage;
    struct vm_execute_expression;

    using vm_executable_unit = boost::variant< boost::recursive_wrapper<vm_block>, vm_allocate_storage, boost::recursive_wrapper<vm_execute_expression> >;

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER
