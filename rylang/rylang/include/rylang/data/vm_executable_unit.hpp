//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER

#include <boost/variant.hpp>

#include "vm_allocate_storage.hpp"
#include "vm_expression.hpp"

namespace rylang
{
    struct vm_block;
    struct vm_allocate_storage;
    struct vm_store
    {
        vm_value what;
        vm_value where;
        vm_type type;
    };
    struct vm_execute_expression
    {
        vm_value expr;
    };

    struct vm_return
    {
        vm_value expr;
    };

    using vm_executable_unit = boost::variant< vm_store, boost::recursive_wrapper< vm_block >, vm_allocate_storage, vm_return >;

} // namespace rylang

#include "vm_block.hpp"

#endif // RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER
