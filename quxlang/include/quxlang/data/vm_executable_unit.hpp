//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_EXECUTABLE_UNIT_HEADER_GUARD
#define QUXLANG_VM_EXECUTABLE_UNIT_HEADER_GUARD

#include <boost/variant.hpp>

#include "quxlang/data/qualified_symbol_reference.hpp"
#include "vm_allocate_storage.hpp"
#include "vm_block.hpp"
#include "vm_expression.hpp"

namespace quxlang
{
    struct vm_block;
    struct vm_allocate_storage;

    struct vm_store
    {
        vm_value what;
        vm_value where;
        type_symbol type;

        auto operator<=>(const vm_store& other) const
        {
            return rpnx::compare(what, other.what, where, other.where, type, other.type);
        }
    };

    struct vm_execute_expression
    {
        vm_value expr;

        std::strong_ordering operator<=>(const vm_execute_expression& other) const
        {
            return rpnx::compare(expr, other.expr);
        }
    };

    struct vm_return
    {
        std::strong_ordering operator<=>(vm_return const&) const = default;
    };

    struct vm_if;
    struct vm_while;

    struct vm_disable_storage
    {
        std::size_t index;
    };

    struct vm_enable_storage
    {
        std::size_t index;
    };

    using vm_executable_unit = rpnx::variant< vm_store, vm_execute_expression, vm_block, vm_return, vm_if, vm_while, vm_disable_storage, vm_enable_storage >;

    struct vm_block
    {
        std::vector< vm_executable_unit > code;
        std::vector< std::string > comments;

        auto operator<=>(const vm_block& other) const
        {
            return rpnx::compare(code, other.code, comments, other.comments);

        }
    };

    struct vm_if
    {
        std::optional< vm_block > condition_block;
        vm_value condition;
        vm_block then_block;
        std::optional< vm_block > else_block;

        std::strong_ordering operator<=>(const vm_if& other) const
        {
            return rpnx::compare(condition_block, other.condition_block, condition, other.condition, then_block, other.then_block, else_block, other.else_block);
        }
    };

    struct vm_while
    {
        std::optional< vm_block > condition_block;
        vm_value condition;
        vm_block loop_block;
    };

} // namespace quxlang

#include "vm_block.hpp"

#endif // QUXLANG_VM_EXECUTABLE_UNIT_HEADER_GUARD