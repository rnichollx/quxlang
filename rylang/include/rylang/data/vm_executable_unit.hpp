//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER

#include <boost/variant.hpp>

#include "rylang/data/qualified_reference.hpp"
#include "vm_allocate_storage.hpp"
#include "vm_block.hpp"
#include "vm_expression.hpp"

namespace rylang
{
    struct vm_block;
    struct vm_allocate_storage;
    struct vm_store
    {
        vm_value what;
        vm_value where;
        qualified_symbol_reference type;
    };
    struct vm_execute_expression
    {
        vm_value expr;
    };

    struct vm_return
    {
        std::optional< vm_value > expr;
    };

    struct vm_if;

    using vm_executable_unit = boost::variant< vm_store, vm_execute_expression, boost::recursive_wrapper< vm_block >, vm_allocate_storage, vm_return, boost::recursive_wrapper< vm_if > >;

    struct vm_block
    {
        std::vector< vm_executable_unit > code;
    };

    struct vm_if
    {
        std::optional<vm_block> condition_block;
        vm_value condition;
        vm_block then_block;
        std::optional< vm_block > else_block;
    };

} // namespace rylang

#include "vm_block.hpp"

#endif // RPNX_RYANSCRIPT1031_VM_EXECUTABLE_UNIT_HEADER
