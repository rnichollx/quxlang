// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_VMIR2_VMIR2_HEADER_GUARD
#define QUXLANG_VMIR2_VMIR2_HEADER_GUARD

#include <cstdint>
#include <map>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/macros.hpp>
#include <rpnx/macros.hpp>
#include <rpnx/variant.hpp>
#include <string>
#include <vector>

#include <quxlang/ast2/source_location.hpp>
#include <rpnx/uint64_base.hpp>

namespace quxlang
{
    struct source_location;
}
RPNX_ENUM(quxlang::vmir2, slot_kind, std::uint16_t, invalid, positional_arg, named_arg, local, literal, symbol, binding);

RPNX_ENUM(quxlang::vmir2, slot_stage, std::uint16_t, dead, partial, full);
RPNX_ENUM(quxlang::vmir2, conversion_class, std::uint16_t, checked, partial, assume);

/// Controls whether constexpr_set_result2 materializes a value slot or the object behind a reference slot.
RPNX_ENUM(quxlang::vmir2, constexpr_result_target_mode, std::uint8_t, value, referenced_object);

namespace quxlang
{
    namespace vmir2
    {
        struct access_field;
        struct access_array;
        struct access_pointer;
        struct ret;
        struct invoke;
        struct invoke_indirect;
        struct get_procedure_ptr;
        struct make_reference;
        struct copy_reference;
        struct jump;
        struct branch;
        struct cast_ptrref;
        struct constexpr_set_result;
        struct constexpr_set_result2;
        struct constexpr_make_proxy;
        struct constexpr_output_byte;
        struct load_const_value;
        struct load_const_int;
        struct load_const_float;
        struct canonicalize_float;
        struct get_value_byte;
        struct set_value_byte;
        struct make_pointer_to;
        struct move_value;
        struct load_from_ref;
        struct store_to_ref;
        struct storage_init;
        struct storage_init_start;
        struct storage_deinit_start;
        struct storage_pun;
        struct constexpr_alloc;
        struct constexpr_alloc_multiple;
        struct constexpr_dealloc;
        struct constexpr_dealloc_multiple;
        struct get_global_storage;
        struct get_antestatal_ref;
        struct initguard_global_get_ref;
        struct initguard_release;
        struct initguard_abort;
        struct dereference_pointer;
        struct load_const_zero;
        struct defer_nontrivial_dtor;
        struct destroy;
        struct end_lifetime;
        struct initguard_try_acquire;
        struct ptr_offset;
        struct ptr_comp;
        struct ptr_to_i;
        struct i_to_ptr;
        struct to_bool;
        struct to_bool_not;
        struct runtime_constexpr;
        struct increment;
        struct decrement;
        struct preincrement;
        struct predecrement;
        struct iconv;

        struct int_add;
        struct int_mul;
        struct int_div;
        struct int_mod;
        struct int_sub;
        struct mut_int_add;
        struct mut_int_sub;
        struct mut_int_mul;
        struct mut_int_div;
        struct mut_int_mod;
        struct float_add;
        struct float_sub;
        struct float_mul;
        struct float_div;
        struct mut_float_add;
        struct mut_float_sub;
        struct mut_float_mul;
        struct mut_float_div;
        struct float_from_int;

        struct cmp_lt;
        struct cmp_ge;
        struct cmp_eq;
        struct cmp_ne;
        struct float_ieee_eq;
        struct float_ieee_ne;
        struct float_ieee_lt;
        struct float_ieee_gt;
        struct pcmp_lt;
        struct pcmp_ge;
        struct pcmp_eq;
        struct pcmp_ne;
        struct gcmp_lt;
        struct gcmp_ge;
        struct gcmp_eq;
        struct gcmp_ne;

        struct bitwise_and;
        struct bitwise_or;
        struct bitwise_xor;
        struct bitwise_nand;
        struct bitwise_nor;
        struct bitwise_nxor;
        struct bitwise_implies;
        struct bitwise_implied;
        struct bitwise_shift_up;
        struct bitwise_shift_down;
        struct bitwise_rotate_up;
        struct bitwise_rotate_down;
        struct bitwise_inverse;
        struct mut_bitwise_and;
        struct mut_bitwise_or;
        struct mut_bitwise_xor;
        struct mut_bitwise_nand;
        struct mut_bitwise_nor;
        struct mut_bitwise_nxor;
        struct mut_bitwise_implies;
        struct mut_bitwise_implied;
        struct mut_bitwise_shift_up;
        struct mut_bitwise_shift_down;
        struct mut_bitwise_rotate_up;
        struct mut_bitwise_rotate_down;

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

        // clang-format: off
        using vm_instruction = rpnx::variant<
            access_field,
            invoke,
            invoke_indirect,
            get_procedure_ptr,
            make_reference,
            cast_ptrref,
            constexpr_set_result,
            constexpr_set_result2,
            constexpr_make_proxy,
            constexpr_output_byte,
            load_const_int,
            load_const_float,
            load_const_value,
            canonicalize_float,
            get_value_byte,
            set_value_byte,
            make_pointer_to,
            load_from_ref,
            storage_init,
            storage_init_start,
            storage_deinit_start,
            storage_pun,
            constexpr_alloc,
            constexpr_alloc_multiple,
            constexpr_dealloc,
            constexpr_dealloc_multiple,
            get_global_storage,
            get_antestatal_ref,
            initguard_global_get_ref,
            initguard_release,
            initguard_abort,
            load_const_zero,
            load_const_bool,
            dereference_pointer,
            store_to_ref,
            int_add,
            int_mul,
            int_div,
            int_mod,
            int_sub,
            mut_int_add,
            mut_int_sub,
            mut_int_mul,
            mut_int_div,
            mut_int_mod,
            float_add,
            float_sub,
            float_mul,
            float_div,
            mut_float_add,
            mut_float_sub,
            mut_float_mul,
            mut_float_div,
            float_from_int,
            iconv,
            bitwise_and,
            bitwise_or,
            bitwise_xor,
            bitwise_nand,
            bitwise_nor,
            bitwise_nxor,
            bitwise_implies,
            bitwise_implied,
            bitwise_shift_up,
            bitwise_shift_down,
            bitwise_rotate_up,
            bitwise_rotate_down,
            bitwise_inverse,
            mut_bitwise_and,
            mut_bitwise_or,
            mut_bitwise_xor,
            mut_bitwise_nand,
            mut_bitwise_nor,
            mut_bitwise_nxor,
            mut_bitwise_implies,
            mut_bitwise_implied,
            mut_bitwise_shift_up,
            mut_bitwise_shift_down,
            mut_bitwise_rotate_up,
            mut_bitwise_rotate_down,
            cmp_lt,
            cmp_ge,
            cmp_eq,
            cmp_ne,
            float_ieee_eq,
            float_ieee_ne,
            float_ieee_lt,
            float_ieee_gt,
            pcmp_lt,
            pcmp_ge,
            pcmp_eq,
            pcmp_ne,
            gcmp_lt,
            gcmp_ge,
            gcmp_eq,
            gcmp_ne,
            defer_nontrivial_dtor,
            struct_init_start,
            struct_init_finish,
            copy_reference,
            destroy,
            end_lifetime,
            access_array,
            access_pointer,
            to_bool,
            to_bool_not,
            increment,
            decrement,
            preincrement,
            predecrement,
            pointer_arith,
            pointer_diff,
            assert_instr,
            swap,
            unimplemented,
            array_init_start,
            array_init_index,
            array_init_element,
            array_init_finish,
            array_init_more
        >;
        // clang-format: on
        using vm_terminator = rpnx::variant< jump, branch, runtime_constexpr, initguard_try_acquire, ret >;

        RPNX_UNIQUE_U64(local_index);

        RPNX_UNIQUE_U64(block_index);

        struct end_lifetime
        {
            local_index of;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(end_lifetime, of);
        };

        struct destroy
        {
            local_index of;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(destroy, of);
        };

        struct unimplemented
        {
            std::optional< std::string > message;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(unimplemented, message);
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

        struct storage_init
        {
            local_index storage;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(storage_init, storage);
        };

        struct storage_init_start
        {
            local_index on_storage;
            local_index target_value;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(storage_init_start, on_storage, target_value);
        };

        struct storage_deinit_start
        {
            local_index on_storage;
            local_index target_value;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(storage_deinit_start, on_storage, target_value);
        };

        struct storage_pun
        {
            local_index from_storage;
            type_symbol as_type;
            local_index to_reference;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(storage_pun, from_storage, as_type, to_reference);
        };

        struct constexpr_alloc
        {
            type_symbol storage_type;
            local_index result;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_alloc, storage_type, result);
        };

        struct constexpr_alloc_multiple
        {
            type_symbol storage_type;
            local_index count;
            local_index result;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_alloc_multiple, storage_type, count, result);
        };

        struct constexpr_dealloc
        {
            type_symbol storage_type;
            local_index pointer;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_dealloc, storage_type, pointer);
        };

        struct constexpr_dealloc_multiple
        {
            type_symbol storage_type;
            local_index pointer;
            local_index count;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_dealloc_multiple, storage_type, pointer, count);
        };

        struct get_global_storage
        {
            type_symbol symbol;
            local_index target_ref;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(get_global_storage, symbol, target_ref);
        };

        struct get_antestatal_ref
        {
            type_symbol symbol;
            local_index target_ref;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(get_antestatal_ref, symbol, target_ref);
        };

        struct initguard_global_get_ref
        {
            type_symbol symbol;
            local_index target_ref;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(initguard_global_get_ref, symbol, target_ref);
        };

        struct initguard_release
        {
            local_index lock;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(initguard_release, lock);
        };

        struct initguard_abort
        {
            local_index lock;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(initguard_abort, lock);
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

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(struct_init_start, on_value, fields);
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

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(struct_init_finish, on_value);
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

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(array_init_start, on_value, initializer);
        };

        // The array_init_element (ARRAY_INIT_ELEMENT) instruction is used to assign a
        // array element that is not yet constructed to a slot.
        struct array_init_element
        {
            local_index initializer;
            local_index target;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(array_init_element, initializer, target);
        };


        // The array_init_index (ARRAY_INIT_INDEX) instruction returns the index of the element being initialized,
        // or the size of the array if completed.
        struct array_init_index
        {
            local_index initializer;
            local_index result;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(array_init_index, initializer, result);
        };

        // The array_init_remaining (ARRAY_INIT_REMAINING) instruction is used to test whether or not there are
        // more elements remaining to initialize.
        struct array_init_more
        {
            local_index initializer;
            local_index result;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(array_init_more, initializer, result);
        };

        // The array_init_finish (ARRAY_INIT_FINISH) instruction is used to finalize
        // an array initialization after all elements have been initialized.
        // This is mostly for cleanup purposes, to mark the initializer slot as no longer needed.
        // If the array initializer is not at the completed position, the program's behavior is undefined.
        struct array_init_finish
        {
            local_index initializer;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(array_init_finish, initializer);
        };

        struct swap
        {
            local_index a;
            local_index b;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(swap, a, b);
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

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(pointer_arith, from, multiplier, offset, result);
        };

        // pointer_diff(PDF) is used to find the offset between two pointers in the same array
        struct pointer_diff
        {
            local_index from;
            local_index to;
            local_index result;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(pointer_diff, from, to, result);
        };

        // We make assert a custom instruction so it can be used during constexpr and cause the
        // constexpr evaluation to fail. Especially for use in STATIC_TEST.
        struct assert_instr
        {
            local_index condition;
            std::string message;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(assert_instr, condition, message);
        };

        // Defers a non-trivial destructor call.
        struct defer_nontrivial_dtor
        {
            type_symbol func;
            local_index on_value;
            invocation_args args;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(defer_nontrivial_dtor, func, on_value, args);
        };

        struct access_field
        {
            local_index base_index = local_index(0);
            local_index store_index = local_index(0);
            std::string field_name;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(access_field, base_index, store_index, field_name);
        };

        struct access_array
        {
            local_index base_index = local_index(0);
            local_index index_index = local_index(0);
            local_index store_index = local_index(0);
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(access_array, base_index, index_index, store_index);
        };

        struct access_pointer
        {
            local_index base_index = local_index(0);
            local_index index_index = local_index(0);
            local_index store_index = local_index(0);
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(access_pointer, base_index, index_index, store_index);
        };

        struct invoke
        {
            type_symbol what;
            invocation_args args;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(invoke, what, args);
        };

        struct invoke_indirect
        {
            local_index what_index = local_index(0);
            invocation_args args;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(invoke_indirect, what_index, args);
        };

        struct get_procedure_ptr
        {
            type_symbol routine;
            std::string calling_convention = "DEFAULT";
            local_index pointer_index = local_index(0);

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(get_procedure_ptr, routine, calling_convention, pointer_index);
        };

        struct copy_reference
        {
            local_index from_index;
            local_index to_index;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(copy_reference, from_index, to_index);
        };

        // MKR makes a reference to a value
        struct make_reference
        {
            local_index value_index;
            local_index reference_index;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(make_reference, value_index, reference_index);
        };

        struct make_pointer_to
        {
            local_index of_index;
            local_index pointer_index;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(make_pointer_to, of_index, pointer_index);
        };

        struct cast_ptrref
        {
            local_index source_index;
            local_index target_index;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(cast_ptrref, source_index, target_index);
        };

        struct constexpr_set_result
        {
            local_index target;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_set_result, target);
        };

        struct constexpr_set_result2
        {
            /// Local whose value or referenced object should be materialized as an antestatal result.
            local_index target;
            /// Result key; zero is the primary expression result and nonzero IDs return updated statics.
            std::uint64_t result_id = 0;
            /// Selects whether target is the object itself or a reference to the object to materialize.
            constexpr_result_target_mode target_mode = constexpr_result_target_mode::value;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_set_result2, target, result_id, target_mode);
        };

        struct constexpr_make_proxy
        {
            local_index target;
            std::uint64_t result_id = 0;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_make_proxy, target, result_id);
        };

        struct constexpr_output_byte
        {
            local_index proxy;
            local_index value;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(constexpr_output_byte, proxy, value);
        };

        struct load_const_value
        {
            local_index target;
            std::vector< std::byte > value;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(load_const_value, target, value);
        };

        struct canonicalize_float
        {
            local_index source;
            local_index result;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(canonicalize_float, source, result);
        };

        struct get_value_byte
        {
            local_index source_reference;
            std::uint64_t offset = 0;
            local_index result;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(get_value_byte, source_reference, offset, result);
        };

        struct set_value_byte
        {
            local_index target_reference;
            std::uint64_t offset = 0;
            local_index value;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(set_value_byte, target_reference, offset, value);
        };

        // Converts a pointer into a reference to the pointed-to value
        struct dereference_pointer
        {
            local_index from_pointer;
            local_index to_reference;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(dereference_pointer, from_pointer, to_reference);
        };

        struct load_from_ref
        {
            local_index from_reference;
            local_index to_value;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(load_from_ref, from_reference, to_value);
        };

        struct store_to_ref
        {
            local_index from_value;
            local_index to_reference;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(store_to_ref, from_value, to_reference);
        };

        struct load_const_int
        {

            local_index target;
            std::string value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(load_const_int, target, value);
        };

        struct load_const_float
        {
            local_index target;
            std::string value;
            bool require_exact = true;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(load_const_float, target, value, require_exact);
        };

        struct load_const_bool
        {
            local_index target;
            bool value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(load_const_bool, target, value);
        };

        struct load_const_zero
        {
            local_index target;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(load_const_zero, target);
        };

        struct int_add
        {

            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(int_add, a, b, result);
        };

        struct int_sub
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(int_sub, a, b, result);
        };

        struct int_mul
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(int_mul, a, b, result);
        };

        struct int_div
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(int_div, a, b, result);
        };

        struct int_mod
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(int_mod, a, b, result);
        };

        struct mut_int_add
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_int_add, target, value);
        };

        struct mut_int_sub
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_int_sub, target, value);
        };

        struct mut_int_mul
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_int_mul, target, value);
        };

        struct mut_int_div
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_int_div, target, value);
        };

        struct mut_int_mod
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_int_mod, target, value);
        };

        struct float_add
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_add, a, b, result);
        };

        struct float_sub
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_sub, a, b, result);
        };

        struct float_mul
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_mul, a, b, result);
        };

        struct float_div
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_div, a, b, result);
        };

        struct mut_float_add
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_float_add, target, value);
        };

        struct mut_float_sub
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_float_sub, target, value);
        };

        struct mut_float_mul
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_float_mul, target, value);
        };

        struct mut_float_div
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_float_div, target, value);
        };

        struct float_from_int
        {
            local_index source;
            local_index result;
            bool require_exact = true;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_from_int, source, result, require_exact);
        };

        struct iconv
        {
            local_index from;
            local_index to;
            conversion_class convtype{};

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(iconv, from, to, convtype);
        };

        // Bitwise operations
        struct bitwise_and
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_and, a, b, result);
        };
        struct bitwise_or
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_or, a, b, result);
        };
        struct bitwise_xor
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_xor, a, b, result);
        };
        struct bitwise_nand
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_nand, a, b, result);
        };
        struct bitwise_nor
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_nor, a, b, result);
        };
        struct bitwise_nxor
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_nxor, a, b, result);
        };
        struct bitwise_implies
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_implies, a, b, result);
        };
        struct bitwise_implied
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_implied, a, b, result);
        };
        struct bitwise_shift_up
        {
            local_index value;
            local_index amount;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_shift_up, value, amount, result);
        };
        struct bitwise_shift_down
        {
            local_index value;
            local_index amount;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_shift_down, value, amount, result);
        };
        struct bitwise_rotate_up
        {
            local_index value;
            local_index amount;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_rotate_up, value, amount, result);
        };
        struct bitwise_rotate_down
        {
            local_index value;
            local_index amount;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_rotate_down, value, amount, result);
        };
        struct bitwise_inverse
        {
            local_index value;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(bitwise_inverse, value, result);
        };

        struct mut_bitwise_and
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_and, target, value);
        };
        struct mut_bitwise_or
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_or, target, value);
        };
        struct mut_bitwise_xor
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_xor, target, value);
        };
        struct mut_bitwise_nand
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_nand, target, value);
        };
        struct mut_bitwise_nor
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_nor, target, value);
        };
        struct mut_bitwise_nxor
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_nxor, target, value);
        };
        struct mut_bitwise_implies
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_implies, target, value);
        };
        struct mut_bitwise_implied
        {
            local_index target;
            local_index value;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_implied, target, value);
        };
        struct mut_bitwise_shift_up
        {
            local_index target;
            local_index amount;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_shift_up, target, amount);
        };
        struct mut_bitwise_shift_down
        {
            local_index target;
            local_index amount;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_shift_down, target, amount);
        };
        struct mut_bitwise_rotate_up
        {
            local_index target;
            local_index amount;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_rotate_up, target, amount);
        };
        struct mut_bitwise_rotate_down
        {
            local_index target;
            local_index amount;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(mut_bitwise_rotate_down, target, amount);
        };

        struct cmp_eq
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(cmp_eq, a, b, result);
        };

        struct pcmp_eq
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(pcmp_eq, a, b, result);
        };

        struct gcmp_eq
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(gcmp_eq, a, b, result);
        };

        struct cmp_ne
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(cmp_ne, a, b, result);
        };

        struct float_ieee_eq
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_ieee_eq, a, b, result);
        };

        struct float_ieee_ne
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_ieee_ne, a, b, result);
        };

        struct float_ieee_lt
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_ieee_lt, a, b, result);
        };

        struct float_ieee_gt
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(float_ieee_gt, a, b, result);
        };

        struct pcmp_ne
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(pcmp_ne, a, b, result);
        };

        struct gcmp_ne
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(gcmp_ne, a, b, result);
        };

        struct cmp_lt
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(cmp_lt, a, b, result);
        };

        struct gcmp_lt
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(gcmp_lt, a, b, result);
        };

        struct pcmp_lt
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(pcmp_lt, a, b, result);
        };

        struct cmp_ge
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(cmp_ge, a, b, result);
        };

        struct pcmp_ge
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(pcmp_ge, a, b, result);
        };

        struct gcmp_ge
        {
            local_index a;
            local_index b;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(gcmp_ge, a, b, result);
        };

        struct to_bool
        {
            local_index from;
            local_index to;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(to_bool, from, to);
        };

        struct to_bool_not
        {
            local_index from;
            local_index to;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(to_bool_not, from, to);
        };

        struct increment
        {
            local_index value;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(increment, value, result);
        };

        struct decrement
        {
            local_index value;
            local_index result;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(decrement, value, result);
        };

        struct preincrement
        {
            local_index target;
            local_index target2;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(preincrement, target, target2);
        };

        struct predecrement
        {
            local_index target;
            local_index target2;
            QUXLANG_WITH_SOURCE_LOCATION_METADATA(predecrement, target, target2);
        };

        struct jump
        {
            block_index target;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(jump, target);
        };

        struct ret
        {
            QUXLANG_WITH_SOURCE_LOCATION_EMPTY_METADATA(ret);
        };

        struct branch
        {
            local_index condition;
            block_index target_true;
            block_index target_false;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(branch, condition, target_true, target_false);
        };

        struct runtime_constexpr
        {
            block_index target_constexpr;
            block_index target_native;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(runtime_constexpr, target_constexpr, target_native);
        };

        struct initguard_try_acquire
        {
            type_symbol symbol;
            local_index target_lock;
            block_index target_acquired;
            block_index target_already_initialized;

            QUXLANG_WITH_SOURCE_LOCATION_METADATA(initguard_try_acquire, symbol, target_lock, target_acquired, target_already_initialized);
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
            bool destroy_delegate = false;
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
                if (destroy_delegate && !alive())
                {
                    return false;
                }
                return true;
            }

            RPNX_MEMBER_METADATA(slot_state, stage, storage_valid, nontrivial_dtor, delegates, delegate_of, destroy_delegate, array_delegate_of_initializer);
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

        struct localdata_entry
        {
            /// Type of the antestatal localdata root.
            type_symbol type;
            /// Initial value loaded into the constexpr interpreter for this root.
            antestatal_value value;
            /// Whether stores through references to this root are permitted.
            bool is_mutable = false;

            RPNX_MEMBER_METADATA(localdata_entry, type, value, is_mutable);
        };

        struct functanoid_routine3
        {
            std::vector< local_type > local_types;
            routine_parameters parameters;
            std::vector< executable_block > blocks;
            std::map< block_index, std::string > block_names;
            std::map< type_symbol, type_symbol > non_trivial_dtors;
            /// Immutable function-local static snapshots carried by this routine.
            std::map< static_snapshot_ref, localdata_entry > static_snapshots;

            RPNX_MEMBER_METADATA(functanoid_routine3, local_types, parameters, blocks, block_names, non_trivial_dtors, static_snapshots);
        };

        struct state_transition
        {
            bool entry_alive = false;
            bool exit_alive = false;

            RPNX_MEMBER_METADATA(state_transition, entry_alive, exit_alive);
        };

        inline std::optional< source_location > get_location(vm_instruction const& instruction)
        {
            return rpnx::apply_visitor< std::optional< source_location > >(instruction, [](auto const& item) { return item.location; });
        }

        inline std::optional< source_location > get_location(vm_terminator const& terminator)
        {
            return rpnx::apply_visitor< std::optional< source_location > >(terminator, [](auto const& item) { return item.location; });
        }
    } // namespace vmir2

}; // namespace quxlang

#endif // RPNX_QUXLANG_VMIR2
