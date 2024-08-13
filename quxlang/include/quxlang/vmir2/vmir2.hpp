//
// Created by Ryan Nicholl on 5/26/24.
//

#ifndef RPNX_QUXLANG_VMIR2_HEADER
#define RPNX_QUXLANG_VMIR2_HEADER

#include <cstdint>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/vm_expression.hpp>
#include <rpnx/metadata.hpp>
#include <rpnx/variant.hpp>
#include <string>
#include <vector>

RPNX_ENUM(quxlang::vmir2, slot_kind, std::uint16_t, arg, local, literal, symbol);

namespace quxlang
{
    namespace vmir2
    {
        struct access_field;
        struct invoke;
        struct make_reference;
        struct jump;
        struct branch;

        using vm_instruction = rpnx::variant< access_field, invoke, make_reference >;
        using vm_terminator = rpnx::variant< jump, branch >;

        using storage_index = std::uint64_t;
        using block_index = std::uint64_t;

        struct invocation_args
        {
            std::map< std::string, storage_index > named;
            std::vector< storage_index > positional;

            inline auto size() const
            {
                return positional.size() + named.size();
            }

            RPNX_MEMBER_METADATA(invocation_args, named, positional);
        };


        struct access_field
        {
            storage_index base_index = 0;
            storage_index store_index = 0;
            size_t offset = 0;

            RPNX_MEMBER_METADATA(access_field, base_index, store_index, offset);
        };

        struct invoke
        {
            type_symbol what;
            invocation_args args;

            RPNX_MEMBER_METADATA(invoke, what, args);
        };

        struct make_reference
        {
            storage_index value_index;
            storage_index reference_index;

            RPNX_MEMBER_METADATA(make_reference, value_index, reference_index);
        };

        struct jump
        {
            block_index target;

            RPNX_MEMBER_METADATA(jump, target);
        };

        struct branch
        {
            storage_index condition;
            block_index target_true;
            block_index target_false;

            RPNX_MEMBER_METADATA(branch, condition, target_true, target_false);
        };

        struct vm_slot
        {
            type_symbol type;
            std::optional< std::string > name;
            std::optional< std::string > literal_value;
            slot_kind kind;

            RPNX_MEMBER_METADATA(vm_slot, type, name, literal_value, kind);
        };

    

        struct vm_context
        {
            std::vector< vm_slot > slots;
        };

        struct functanoid_routine
        {
            std::vector< vm_slot > slots;
            std::vector< vm_instruction > instructions;

            RPNX_MEMBER_METADATA(functanoid_routine, slots, instructions);
        };

        struct slot_state
        {
            bool alive = false;

            RPNX_MEMBER_METADATA(slot_state, alive);
        };

        struct executable_block
        {
            std::vector< slot_state > entry_state;
            std::vector< vm_instruction > instructions;
            std::optional<vm_terminator> terminator;

            RPNX_MEMBER_METADATA(executable_block, entry_state, instructions, terminator);
        };

        struct functanoid_routine2
        {
            std::vector< vm_slot > slots;
            block_index entry_block;
            std::optional< block_index > return_block;
            std::vector< executable_block > blocks;

            RPNX_MEMBER_METADATA(functanoid_routine2, slots, entry_block, return_block, blocks);
        };
    } // namespace vmir2

}; // namespace quxlang

#endif // RPNX_QUXLANG_VMIR2_HEADER
