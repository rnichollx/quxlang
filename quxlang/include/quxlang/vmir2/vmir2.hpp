//
// Created by Ryan Nicholl on 5/26/24.
//

#ifndef QUXLANG_VMIR2_VMIR2_HEADER_GUARD
#define QUXLANG_VMIR2_VMIR2_HEADER_GUARD

#include <cstdint>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/vm_expression.hpp>
#include <rpnx/metadata.hpp>
#include <rpnx/variant.hpp>
#include <string>
#include <vector>

RPNX_ENUM(quxlang::vmir2, slot_kind, std::uint16_t, invalid, positional_arg, named_arg, local, literal, symbol, binding);

namespace quxlang
{
    namespace vmir2
    {
        struct access_field;
        struct ret;
        struct invoke;
        struct make_reference;
        struct jump;
        struct branch;
        struct cast_reference;

        using vm_instruction = rpnx::variant< access_field, invoke, make_reference, cast_reference >;
        using vm_terminator = rpnx::variant< jump, branch, ret >;

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

        struct cast_reference
        {
            storage_index source_ref_index;
            storage_index target_ref_index;
            std::int64_t offset;

            RPNX_MEMBER_METADATA(cast_reference, source_ref_index, target_ref_index, offset);
        };

        struct jump
        {
            block_index target;

            RPNX_MEMBER_METADATA(jump, target);
        };

        struct ret
        {
            RPNX_EMPTY_METADATA(ret);
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
            std::optional< std::string > arg_name;
            std::optional< storage_index > binding_of;
            slot_kind kind;

            RPNX_MEMBER_METADATA(vm_slot, type, name, literal_value, arg_name, binding_of, kind);
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

        struct slot_states
        {
            std::map<std::size_t, bool> alive;

            RPNX_MEMBER_METADATA(slot_states, alive);
        };

        struct executable_block
        {
            std::map< std::size_t, slot_state > entry_state;
            std::vector< vm_instruction > instructions;
            std::optional< vm_terminator > terminator;

            RPNX_MEMBER_METADATA(executable_block, entry_state, instructions, terminator);
        };

        struct slot_generation_state
        {

            slot_generation_state()
            {
                slots.push_back(vmir2::vm_slot{.type = void_type{}, .name = "VOID", .literal_value = "VOID", .kind = vmir2::slot_kind::literal});
            }

            slot_generation_state(const slot_generation_state&) = default;
            slot_generation_state(slot_generation_state&&) = default;

            slot_generation_state& operator=(const slot_generation_state&) = default;
            slot_generation_state& operator=(slot_generation_state&&) = default;

            std::vector< vm_slot > slots;

            storage_index create_temporary(type_symbol type);
            storage_index create_variable(type_symbol type, std::string name);
            storage_index create_binding(storage_index idx, type_symbol type);
            storage_index create_positional_argument(type_symbol type);
            storage_index create_named_argument(std::string name, type_symbol type);
            storage_index create_numeric_literal(std::string value);
            storage_index index_binding(storage_index idx);

            RPNX_MEMBER_METADATA(slot_generation_state, slots);
        };

        struct executable_block_generation_state
        {

            executable_block_generation_state(slot_generation_state *slots) : slots(slots)
            {
            }
            executable_block_generation_state(const executable_block_generation_state&) = default;
            executable_block_generation_state(executable_block_generation_state&&) = default;


            vmir2::executable_block block;
            slot_generation_state *slots;
            std::map< std::size_t, slot_state > current_slot_states = {{0,slot_state{}}};
            std::map< std::string , storage_index > named_references;

            type_symbol current_type(storage_index idx);

            executable_block_generation_state clone_subblock();
            void emit(vmir2::access_field fld);
            void emit(vmir2::invoke inv);
            void emit(vmir2::cast_reference cst);
            void emit(vmir2::make_reference cst);
            bool slot_alive(storage_index idx);

            storage_index create_temporary(type_symbol type);
            storage_index create_variable(type_symbol type, std::string name);
            storage_index create_binding(storage_index idx, type_symbol type);
            storage_index create_positional_argument(type_symbol type, std::optional< std::string > label_name);
            storage_index create_named_argument(std::string interface_name, type_symbol type, std::optional< std::string > label_name);


            storage_index create_numeric_literal(std::string value);
            storage_index index_binding(storage_index idx);

            std::optional<storage_index> local_lookup(std::string name);

            RPNX_MEMBER_METADATA(executable_block_generation_state, block, current_slot_states);
        };

        struct functanoid_routine2
        {
            std::vector< vm_slot > slots;
            block_index entry_block = 0;
            std::optional< block_index > return_block;
            std::vector< executable_block > blocks;
            std::map< block_index, std::string > block_names;

            RPNX_MEMBER_METADATA(functanoid_routine2, slots, entry_block, return_block, blocks, block_names);
        };

        struct frame_generation_state
        {
            slot_generation_state slots;

            std::vector<vmir2::executable_block_generation_state> block_states;
            std::map<std::string, std::size_t> block_map;
            std::optional<std::size_t> entry_block_opt;

            void generate_jump(std::size_t from, std::size_t to);
            void generate_branch(std::size_t condition, std::size_t from, std::size_t true_branch, std::size_t false_branch);
            void generate_return(std::size_t from);
            inline bool has_terminator(std::size_t block)
            {
                return block_states[block].block.terminator.has_value();
            }
            std::size_t generate_entry_block();
            std::size_t generate_subblock(std::size_t of);

            std::size_t entry_block_id();

            executable_block_generation_state & entry_block();
            executable_block_generation_state & block(std::size_t id);

            std::optional<storage_index> lookup(std::size_t block_id, std::string name);

            functanoid_routine2 get_result();

            RPNX_MEMBER_METADATA(frame_generation_state, slots);

        };


    } // namespace vmir2

}; // namespace quxlang

#endif // RPNX_QUXLANG_VMIR2