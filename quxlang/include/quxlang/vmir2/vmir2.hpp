// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_VMIR2_VMIR2_HEADER_GUARD
#define QUXLANG_VMIR2_VMIR2_HEADER_GUARD

#include <cstdint>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/vm_expression.hpp>
#include <rpnx/metadata.hpp>
#include <rpnx/variant.hpp>
#include <string>
#include <vector>

#include <quxlang/ast2/source_location.hpp>
#include <rpnx/uint64_base.hpp>

namespace quxlang
{
    struct ast2_source_location;
}
RPNX_ENUM(quxlang::vmir2, slot_kind, std::uint16_t, invalid, positional_arg, named_arg, local, literal, symbol, binding);

RPNX_ENUM(quxlang::vmir2, slot_stage, std::uint16_t, dead, partial, full);

namespace quxlang
{
    namespace vmir2
    {
        struct access_field;
        struct access_array;
        struct ret;
        struct invoke;
        struct make_reference;
        struct copy_reference;
        struct jump;
        struct branch;
        struct cast_reference;
        struct constexpr_set_result;
        struct load_const_value;
        struct load_const_int;
        struct make_pointer_to;
        struct move_value;
        struct load_from_ref;
        struct store_to_ref;
        struct dereference_pointer;
        struct load_const_zero;
        struct defer_nontrivial_dtor;
        struct end_lifetime;
        struct ptr_offset;
        struct ptr_comp;
        struct ptr_to_i;
        struct i_to_ptr;
        struct to_bool;
        struct to_bool_not;
        struct runtime_ce;
        struct increment;
        struct decrement;
        struct preincrement;
        struct predecrement;

        struct int_add;
        struct int_mul;
        struct int_div;
        struct int_mod;
        struct int_sub;

        struct cmp_lt;
        struct cmp_ge;
        struct cmp_eq;
        struct cmp_ne;
        struct pcmp_lt;
        struct pcmp_ge;
        struct pcmp_eq;
        struct pcmp_ne;
        struct gcmp_lt;
        struct gcmp_ge;
        struct gcmp_eq;
        struct gcmp_ne;

        struct struct_init_start;
        struct struct_init_finish;
        struct fence_byte_release;
        struct fence_byte_acquire;
        struct load_const_bool;

        struct assert_instr;

        struct pointer_arith;
        struct pointer_diff;
        struct swap;
        struct unimplemented;

        struct array_init_start;
        struct array_init_index;
        struct array_init_element;
        struct array_init_finish;
        struct array_init_more;

        using vm_instruction = rpnx::variant< access_field, invoke, make_reference, cast_reference, constexpr_set_result, load_const_int, load_const_value, make_pointer_to, load_from_ref, load_const_zero, load_const_bool, dereference_pointer, store_to_ref, int_add, int_mul, int_div, int_mod, int_sub, cmp_lt, cmp_ge, cmp_eq, cmp_ne, pcmp_lt, pcmp_ge, pcmp_eq, pcmp_ne, gcmp_lt, gcmp_ge, gcmp_eq, gcmp_ne, defer_nontrivial_dtor, struct_init_start, struct_init_finish, copy_reference, end_lifetime, access_array, to_bool, to_bool_not, runtime_ce, increment, decrement, preincrement, predecrement, pointer_arith, pointer_diff, assert_instr, swap, unimplemented, array_init_start, array_init_index, array_init_element, array_init_finish, array_init_more >;
        using vm_terminator = rpnx::variant< jump, branch, ret >;

        RPNX_UNIQUE_U64(local_index);

        RPNX_UNIQUE_U64(block_index);

        struct end_lifetime
        {
            local_index of;

            RPNX_MEMBER_METADATA(end_lifetime, of);
        };

        struct unimplemented
        {
            std::optional< std::string > message;
            RPNX_MEMBER_METADATA(unimplemented, message);
        };

        struct newtype
        {
            RPNX_EMPTY_METADATA(newtype);
        };

        struct invocation_args
        {
            std::map< std::string, local_index > named;
            std::vector< local_index > positional;

            inline auto size() const
            {
                return positional.size() + named.size();
            }

            RPNX_MEMBER_METADATA(invocation_args, named, positional);
        };

        // The struct_init_start (STRUCT_INIT_START) instruction is used in constructor and destructor delegation to member fields.
        // It doesn't do anything per se but affects how the stack unwinds and value lifetimes are managed.
        // Given a value like "x" of struct type, the delegates can be e.g. x.y, x.z, x.w, etc.
        // Thus the struct_init_start instruction allows assignment of struct fields to slots, so that those fields are
        // considered part of the unwind process. The delegate instruction is the only way to begin the lifetime of
        // a struct type. I.e. it transitions a struct into the "partially constructed" state.
        // The slot type of the delegates should be of the same type as the fields, omitting NEW&.
        struct struct_init_start
        {
            local_index on_value;
            invocation_args fields;

            RPNX_MEMBER_METADATA(struct_init_start, on_value, fields);
        };

        // struct_init_finish is used in conjunction with struct_init_start to finalize an object.
        // A struct's destructor becomes primed for activation after SIF.
        // Between SIS and SIF, only the field destructors will run.
        // SIF is called immediately before the constructor body executes after delegate
        // initialization completes.
        // This is only needed if an invocation is inlined in the QXVMIR, a constructor returning implicitly finalizes
        // the struct.
        struct struct_init_finish
        {
            local_index on_value;

            RPNX_MEMBER_METADATA(struct_init_finish, on_value);
        };

        // The array_init_start (ARRAY_INIT_START) instruction is used to mark the beginning of
        // an array initialization. It constructs an array initializer `__ARRAY_INITIALIZER(N, T)`
        // for the array located at 'on_value'.
        // The `__ARRAY_INITIALIZER(N, T)` type has a special interaction with invoke and the lifetime
        // tracking/unwinding, namely it provides runtime lifetime tracking support for a potentially
        // unlimited number of array elements.
        // Calling IVK on an array initializer with a new slot advances the array initialization by one element,
        // constructing the next element in place.
        // When unwinding occurs, the array initializer will destruct the subset of the initialized elements.
        struct array_init_start
        {
            local_index on_value;
            local_index initializer;

            RPNX_MEMBER_METADATA(array_init_start, on_value, initializer);
        };

        // The array_init_element (ARRAY_INIT_ELEMENT) instruction is used to assign a
        // array element that is not yet constructed to a slot.
        struct array_init_element
        {
            local_index initializer;
            local_index target;

            RPNX_MEMBER_METADATA(array_init_element, initializer, target);
        };


        // The array_init_index (ARRAY_INIT_INDEX) instruction returns the index of the element being initialized,
        // or the size of the array if completed.
        struct array_init_index
        {
            local_index initializer;
            local_index result;

            RPNX_MEMBER_METADATA(array_init_index, initializer, result);
        };

        // The array_init_remaining (ARRAY_INIT_REMAINING) instruction is used to test whether or not there are
        // more elements remaining to initialize.
        struct array_init_more
        {
            local_index initializer;
            local_index result;

            RPNX_MEMBER_METADATA(array_init_more, initializer, result);
        };

        // The array_init_finish (ARRAY_INIT_FINISH) instruction is used to finalize
        // an array initialization after all elements have been initialized.
        // This is mostly for cleanup purposes, to mark the initializer slot as no longer needed.
        // If the array initializer is not at the completed position, the program's behavior is undefined.
        struct array_init_finish
        {
            local_index initializer;

            RPNX_MEMBER_METADATA(array_init_finish, initializer);
        };

        struct swap
        {
            local_index a;
            local_index b;

            RPNX_MEMBER_METADATA(swap, a, b);
        };

        // fence_byte_acquire (FBA) causes values of type BYTE that were written to memory to be visible
        // in a subsequent load of type ".type". If the ".type" is VOID then it acts as a global barrier for all types.
        struct fence_byte_acquire
        {
            type_symbol type;

            RPNX_MEMBER_METADATA(fence_byte_acquire, type);
        };

        // fence_byte_release (FBR) causes values of type ".type" written to memory to become visible to subsequent load operations
        // of type BYTE.
        struct fence_byte_release
        {
            type_symbol type;

            RPNX_MEMBER_METADATA(fence_byte_release, type);
        };

        // pointer_arith (PAR) is used to calculate the offset of a pointer.
        // It is used to calculate the offset of a pointer to a struct member.
        // args:
        // 1. from: the array pointer to start from
        // 2. multiplier: either 1 or -1, depending on if used for addition or subtraction
        //   Note: The size of the type is implicit in the type of 'from'
        // 3. offset: the offset of the element relative to the starting pointer
        // 4. result: the result of the pointer offset calculation
        struct pointer_arith
        {
            local_index from;
            std::int64_t multiplier;
            local_index offset;
            local_index result;

            RPNX_MEMBER_METADATA(pointer_arith, from, multiplier, offset, result);
        };

        // pointer_diff(PDF) is used to find the offset between two pointers in the same array
        struct pointer_diff
        {
            local_index from;
            local_index to;
            local_index result;

            RPNX_MEMBER_METADATA(pointer_diff, from, to, result);
        };

        // We make assert a custom instruction so it can be used during constexpr and cause the
        // constexpr evaluation to fail. Especially for use in STATIC_TEST.
        struct assert_instr
        {
            local_index condition;
            std::string message;
            std::optional< ast2_source_location > location;

            RPNX_MEMBER_METADATA(assert_instr, condition, message, location);
        };

        // Defers a non-trivial destructor call.
        struct defer_nontrivial_dtor
        {
            type_symbol func;
            local_index on_value;
            invocation_args args;

            RPNX_MEMBER_METADATA(defer_nontrivial_dtor, func, on_value, args);
        };

        struct access_field
        {
            local_index base_index = local_index(0);
            local_index store_index = local_index(0);
            std::string field_name;

            RPNX_MEMBER_METADATA(access_field, base_index, store_index, field_name);
        };

        struct access_array
        {
            local_index base_index = local_index(0);
            local_index index_index = local_index(0);
            local_index store_index = local_index(0);
            RPNX_MEMBER_METADATA(access_array, base_index, index_index, store_index);
        };

        struct invoke
        {
            type_symbol what;
            invocation_args args;

            RPNX_MEMBER_METADATA(invoke, what, args);
        };

        struct copy_reference
        {
            local_index from_index;
            local_index to_index;

            RPNX_MEMBER_METADATA(copy_reference, from_index, to_index);
        };

        // MKR makes a reference to a value
        struct make_reference
        {
            local_index value_index;
            local_index reference_index;

            RPNX_MEMBER_METADATA(make_reference, value_index, reference_index);
        };

        struct make_pointer_to
        {
            local_index of_index;
            local_index pointer_index;

            RPNX_MEMBER_METADATA(make_pointer_to, of_index, pointer_index);
        };

        struct cast_reference
        {
            local_index source_ref_index;
            local_index target_ref_index;

            RPNX_MEMBER_METADATA(cast_reference, source_ref_index, target_ref_index);
        };

        struct constexpr_set_result
        {
            local_index target;
            RPNX_MEMBER_METADATA(constexpr_set_result, target);
        };

        struct load_const_value
        {
            local_index target;
            std::vector< std::byte > value;

            RPNX_MEMBER_METADATA(load_const_value, target, value);
        };

        // Converts a pointer into a reference to the pointed-to value
        struct dereference_pointer
        {
            local_index from_pointer;
            local_index to_reference;

            RPNX_MEMBER_METADATA(dereference_pointer, from_pointer, to_reference);
        };

        struct load_from_ref
        {
            local_index from_reference;
            local_index to_value;

            RPNX_MEMBER_METADATA(load_from_ref, from_reference, to_value);
        };

        struct store_to_ref
        {
            local_index from_value;
            local_index to_reference;

            RPNX_MEMBER_METADATA(store_to_ref, from_value, to_reference);
        };

        struct load_const_int
        {

            local_index target;
            std::string value;
            RPNX_MEMBER_METADATA(load_const_int, target, value);
        };

        struct load_const_bool
        {
            local_index target;
            bool value;
            RPNX_MEMBER_METADATA(load_const_bool, target, value);
        };

        struct load_const_zero
        {
            local_index target;
            RPNX_MEMBER_METADATA(load_const_zero, target);
        };

        struct int_add
        {

            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(int_add, a, b, result);
        };

        struct int_sub
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(int_sub, a, b, result);
        };

        struct int_mul
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(int_mul, a, b, result);
        };

        struct int_div
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(int_div, a, b, result);
        };

        struct int_mod
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(int_mod, a, b, result);
        };

        struct cmp_eq
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(cmp_eq, a, b, result);
        };

        struct pcmp_eq
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(pcmp_eq, a, b, result);
        };

        struct gcmp_eq
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(gcmp_eq, a, b, result);
        };

        struct cmp_ne
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(cmp_ne, a, b, result);
        };

        struct pcmp_ne
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(pcmp_ne, a, b, result);
        };

        struct gcmp_ne
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(gcmp_ne, a, b, result);
        };

        struct cmp_lt
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(cmp_lt, a, b, result);
        };

        struct gcmp_lt
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(gcmp_lt, a, b, result);
        };

        struct pcmp_lt
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(pcmp_lt, a, b, result);
        };

        struct cmp_ge
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(cmp_ge, a, b, result);
        };

        struct pcmp_ge
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(pcmp_ge, a, b, result);
        };

        struct gcmp_ge
        {
            local_index a;
            local_index b;
            local_index result;
            RPNX_MEMBER_METADATA(gcmp_ge, a, b, result);
        };

        struct to_bool
        {
            local_index from;
            local_index to;

            RPNX_MEMBER_METADATA(to_bool, from, to);
        };

        struct to_bool_not
        {
            local_index from;
            local_index to;
            RPNX_MEMBER_METADATA(to_bool_not, from, to);
        };

        // The runtime_ce(RT_CE) instruction stores the value TRUE in the target when executed in
        // a constexpr context, or FALSE when executed natively.
        struct runtime_ce
        {
            // TODO: Instead of output a bool, this should be similar to a branch terminator instead,
            // that would simplify codegen and allow the compiler to skip generating code for both paths.
            local_index target;
            RPNX_MEMBER_METADATA(runtime_ce, target);
        };

        struct increment
        {
            local_index value;
            local_index result;
            RPNX_MEMBER_METADATA(increment, value, result);
        };

        struct decrement
        {
            local_index value;
            local_index result;
            RPNX_MEMBER_METADATA(decrement, value, result);
        };

        struct preincrement
        {
            local_index target;
            local_index target2;
            RPNX_MEMBER_METADATA(preincrement, target, target2);
        };

        struct predecrement
        {
            local_index target;
            local_index target2;
            RPNX_MEMBER_METADATA(predecrement, target, target2);
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
            local_index condition;
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
            std::optional< local_index > binding_of;
            slot_kind kind;

            RPNX_MEMBER_METADATA(vm_slot, type, name, literal_value, arg_name, binding_of, kind);
        };

        struct local_type
        {
            type_symbol type;

            RPNX_MEMBER_METADATA(local_type, type);
        };

        struct routine_parameter
        {
            type_symbol type;
            local_index local_index;

            RPNX_MEMBER_METADATA(routine_parameter, type, local_index);
        };

        struct routine_parameters
        {
            std::vector< routine_parameter > positional;
            std::map< std::string, routine_parameter > named;

            RPNX_MEMBER_METADATA(routine_parameters, positional, named);
        };

        struct vm_context
        {
            std::vector< vm_slot > slots;
        };

        struct dtor_spec
        {
            type_symbol func;
            invocation_args args;

            RPNX_MEMBER_METADATA(dtor_spec, func, args);
        };

        struct slot_state
        {
            slot_stage stage = slot_stage::dead;
            bool storage_valid = false;
            std::optional< dtor_spec > nontrivial_dtor;
            std::optional< invocation_args > delegates;
            std::optional< local_index > delegate_of;
            std::optional< local_index > array_delegate_of_initializer;

            bool alive() const
            {
                return stage != slot_stage::dead;
            }
            bool dtor_enabled() const
            {
                return stage == slot_stage::full;
            }
            bool valid() const
            {
                if (alive() && !storage_valid)
                {
                    return false;
                }
                if (stage == slot_stage::dead && (delegates.has_value() || dtor_enabled()))
                {
                    return false;
                }
                if (!storage_valid && delegate_of.has_value())
                {
                    return false;
                }
                if (nontrivial_dtor.has_value() && !dtor_enabled())
                {
                    return false;
                }
                return true;
            }

            RPNX_MEMBER_METADATA(slot_state, stage, storage_valid, nontrivial_dtor, delegates, delegate_of, array_delegate_of_initializer);
        };

        struct slot_states
        {
            std::map< std::size_t, bool > alive;

            RPNX_MEMBER_METADATA(slot_states, alive);
        };

        struct executable_block
        {
            std::map< local_index, slot_state > entry_state;
            std::vector< vm_instruction > instructions;
            std::optional< vm_terminator > terminator;
            std::optional< std::string > dbg_name;

            RPNX_MEMBER_METADATA(executable_block, entry_state, instructions, terminator, dbg_name);
        };

        struct functanoid_routine3
        {
            std::vector< local_type > local_types;
            routine_parameters parameters;
            std::vector< executable_block > blocks;
            std::map< block_index, std::string > block_names;
            std::map< type_symbol, type_symbol > non_trivial_dtors;

            RPNX_MEMBER_METADATA(functanoid_routine3, local_types, parameters, blocks, block_names, non_trivial_dtors);
        };

        struct state_transition
        {
            bool entry_alive = false;
            bool exit_alive = false;

            RPNX_MEMBER_METADATA(state_transition, entry_alive, exit_alive);
        };
    } // namespace vmir2

}; // namespace quxlang

#endif // RPNX_QUXLANG_VMIR2
