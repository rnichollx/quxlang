// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXECUTABLE_UNIT_HEADER_GUARD
#define QUXLANG_DATA_VM_EXECUTABLE_UNIT_HEADER_GUARD



#include "quxlang/data/type_symbol.hpp"
#include "rpnx/metadata.hpp"
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

        RPNX_MEMBER_METADATA(vm_store, what, where, type);
    };

    struct vm_execute_expression
    {
        vm_value expr;

        RPNX_MEMBER_METADATA(vm_execute_expression, expr);
    };

    struct vm_return
    {
        RPNX_MEMBER_METADATA(vm_return)
    };

    struct vm_if;
    struct vm_while;

    struct vm_disable_storage
    {
        std::size_t index;

        RPNX_MEMBER_METADATA(vm_disable_storage, index);
    };

    struct vm_enable_storage
    {
        std::size_t index;

        RPNX_MEMBER_METADATA(vm_enable_storage, index);
    };

    using vm_executable_unit = rpnx::variant< vm_store, vm_invoke, vm_execute_expression, vm_block, vm_return, vm_if, vm_while, vm_disable_storage, vm_enable_storage >;

    struct vm_exec_access_field;

    using vm_executable_unit2 = rpnx::variant< vm_exec_access_field >;

    struct vm_block
    {
        std::vector< vm_executable_unit > code;
        std::vector< std::string > comments;

        RPNX_MEMBER_METADATA(vm_block, code, comments);
    };

    struct vm_if
    {
        std::optional< vm_block > condition_block;
        vm_value condition;
        vm_block then_block;
        std::optional< vm_block > else_block;

        RPNX_MEMBER_METADATA(vm_if, condition_block, condition, then_block, else_block);
    };

    struct vm_while
    {
        std::optional< vm_block > condition_block;
        vm_value condition;
        vm_block loop_block;

        RPNX_MEMBER_METADATA(vm_while, condition_block, condition, loop_block);
    };

} // namespace quxlang

#include "vm_block.hpp"

#endif // QUXLANG_VM_EXECUTABLE_UNIT_HEADER_GUARD