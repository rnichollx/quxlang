// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <utility>

#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

#include "quxlang/backends/asm/arm_asm_converter.hpp"
#include "quxlang/bytemath.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/fixed_bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/parsers/parse_int.hpp"
#include "quxlang/vmir2/assembler.hpp"
#include "rpnx/unimplemented.hpp"

#include <deque>
#include <iostream>

namespace quxlang
{
    struct interp_addr
    {
        cow< type_symbol > func;
        vmir2::block_index block = {};
        std::size_t instruction_index = {};

        RPNX_MEMBER_METADATA(interp_addr, func, block, instruction_index);
    };
} // namespace quxlang

class quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl
{
    friend class quxlang::vmir2::ir2_constexpr_interpreter;

  private:
    struct local;

    std::size_t exec_mode = 1;
    std::map< cow< type_symbol >, class_layout > class_layouts;
    std::map< cow< type_symbol >, cow< functanoid_routine3 > > functanoids3;
    std::vector< std::byte > constexpr_result_v;
    std::optional< type_symbol > constexpr_result_type;
    std::optional< type_symbol > constexpr_result_type_value;
    std::optional< type_symbol > constexpr_result_global_symbol;
    /// Declared type for each global or localdata antestatal root known to this evaluation.
    std::map< type_symbol, type_symbol > constexpr_antestatal_global_types;
    /// Initial antestatal value for each global or localdata root known to this evaluation.
    std::map< type_symbol, antestatal_value > constexpr_antestatal_global_values;
    /// Mutability policy for each global or localdata root known to this evaluation.
    std::map< type_symbol, bool > constexpr_antestatal_global_mutable;
    std::weak_ptr< local > constexpr_result_root;
    std::optional< antestatal_value > constexpr_result_antestatal;
    /// Materialized constexpr_set_result2 outputs keyed by result ID.
    std::map< std::uint64_t, antestatal_value > constexpr_result_antestatal_values;
    std::map< std::uint64_t, constexpr_serialoid > constexpr_result_serialoid_values;
    std::optional< source_index > printer_source_index;

    std::set< type_symbol > missing_functanoids_val;
    std::set< type_symbol > missing_antestatal_globals_val;

    std::uint64_t next_object_id = 1;

    enum class initguard_state : std::uint8_t {
        uninitialized = 0,
        initializing = 1,
        initialized = 2,
    };

    struct primitive_object;
    struct array_object;
    struct struct_object;
    struct pref_object;

    using object = rpnx::variant< primitive_object, array_object, struct_object, pref_object >;

    struct pointer_impl
    {
        // Implmentation of a pointer, there are 4 valid states:
        // 1. pointer_target is set, one_past_the_end is not set: A normal pointer to an object
        // 2. one past the end is set, pointer_target is not set: A one-past-the-end pointer
        // 3. neither are set, nullptr
        // 4. invalidated is set, this is an invalidated pointer.

        // note: the "invalidated" state is invalid from the perspective of the abstract machine
        // interpreter, but a valid state of the C++ object itself.
        // Any state besides these 4 indicates a compiler bug.

        std::optional< std::weak_ptr< local > > pointer_target;
        std::optional< std::weak_ptr< local > > one_past_the_end;
        bool invalidated = false;
    };

    struct object_pointer_impl
    {
        std::weak_ptr< object > pointer_target;
        std::optional< std::int64_t > invalid_offset;
        bool invalidated = false;
    };

    struct interface_object
    {
        type_symbol interface_type;
        std::map< interface_slot_key, type_symbol > functions;
        bool is_default = false;
    };

    struct local
    {
        std::vector< std::byte > data;
        bool negative;
        slot_stage stage = slot_stage::dead;
        bool storage_initiated = false;
        std::uint64_t object_id{};
        bool readonly = false;
        std::optional< type_symbol > procedure;
        std::optional< interface_object > interface_value;
        std::optional< pointer_impl > ref;
        std::optional< std::weak_ptr< local > > member_of;
        std::optional< std::weak_ptr< local > > storage_owner;
        std::optional< std::weak_ptr< local > > initializer_of;
        std::optional< std::weak_ptr< local > > array_init_member_of;
        std::optional< type_symbol > antestatal_static_symbol;
        std::optional< std::uint64_t > constexpr_proxy_output_id;
        std::optional< dtor_spec > dtor;
        std::vector< std::shared_ptr< local > > array_members;
        std::map< std::string, std::shared_ptr< local > > struct_members;
        std::optional< invocation_args > delegates;
        std::shared_ptr< local > stored_object;
        std::optional< type_symbol > storage_active_type;
        std::optional< type_symbol > storage_projection_type;
        bool storage_destroy_delegate = false;
        std::uint64_t storage_alignment = 1;
        std::uint64_t init_count = 0;

        bool alive() const
        {
            return stage != slot_stage::dead;
        }

        bool dtor_enabled() const
        {
            return stage == slot_stage::full;
        }
    };

    struct constexpr_allocation
    {
        builtin_allocator_kind kind;
        type_symbol storage_type;
        std::uint64_t count = 0;
        bool freed = false;
        std::shared_ptr< local > root;
        std::vector< std::shared_ptr< local > > elements;
    };

    struct object_base
    {
        std::weak_ptr< object > member_of;
        std::uint64_t object_id{};
        std::optional< dtor_spec > dtor;
        bool alive = false;
        bool storage_initiated = false;
    };

    using primitive_value = rpnx::variant< bool, bytemath::sle_int_unlimited, bytemath::ule_int_unlimited >;

    struct primitive_object : public object_base
    {
        std::vector< std::byte > data;
        bool is_negative = false;
        bool is_unspecified = false;
        std::size_t bits = 0;
    };

    struct array_object : public object_base
    {
        std::vector< std::shared_ptr< object > > array_members;
    };

    struct struct_object : public object_base
    {
        std::map< std::string, std::shared_ptr< object > > struct_members;
    };

    struct pref_object : public object_base
    {
        std::optional< pointer_impl > ref;
    };

    struct stack_frame
    {
        cow< type_symbol > type;
        cow< functanoid_routine3 > ir3;
        interp_addr address;

        std::map< local_index, std::shared_ptr< local > > local_values;

        bool slot_has_storage(local_index index)
        {
            return local_values[index] && local_values[index]->storage_initiated;
        }

        bool slot_alive(local_index index)
        {
            bool alive = local_values[index] && local_values[index]->alive();
            if (alive)
            {
                assert(slot_has_storage(index));
            }
            return alive;
        }
    };

    std::deque< stack_frame > stack;
    std::map< std::vector< std::byte >, std::shared_ptr< local > > global_constdata;
    std::map< type_symbol, std::shared_ptr< local > > global_storages;
    std::map< local*, type_symbol > global_storage_symbols;
    std::map< type_symbol, std::shared_ptr< local > > antestatal_global_roots;
    std::map< type_symbol, std::shared_ptr< local > > global_initguards;
    std::map< type_symbol, std::shared_ptr< local > > m_procedures;
    std::vector< constexpr_allocation > constexpr_allocations;
    std::map< local*, std::size_t > constexpr_allocation_lookup;

    void call_func(cow< type_symbol > functype, vmir2::invocation_args args);
    type_symbol load_indirect_callable_symbol(local_index slot, bool consume);
    void exec();
    void exec3();

    quxlang::vmir2::state_diff get_state_diff();
    void exec_instr();
    void exec_instr3();

    void raise_fault(std::string const& fault_name);

    type_symbol get_local_type(local_index slot);
    type_symbol get_local_type(std::size_t frame, local_index slot);

    stack_frame& get_current_frame()
    {
        return stack.back();
    }

    stack_frame& current_frame()
    {
        return stack.back();
    }

    std::shared_ptr< local >& current_local(local_index slot)
    {
        return get_current_frame().local_values[slot];
    }

    std::size_t current_frame_index()
    {
        return stack.size() - 1;
    }

    enum class fixed_int_instruction : std::uint8_t {
        add,
        sub,
        mul,
        div,
        mod,
    };

    enum class fixed_float_instruction : std::uint8_t {
        add,
        sub,
        mul,
        div,
    };

    using fixed_int_binary_op = bytemath::int_result (*)(bytemath::fixed_int_options, std::vector< std::byte >, std::vector< std::byte >);
    using fixed_float_binary_op = bytemath::float_result (*)(bytemath::fixed_float_options, std::vector< std::byte >, std::vector< std::byte >);
    using fixed_float_compare_op = bytemath::bool_result (*)(bytemath::fixed_float_options, std::vector< std::byte >, std::vector< std::byte >);
    using mut_bitwise_binary_op = std::uint8_t (*)(std::uint8_t, std::uint8_t);

    std::size_t get_type_size(const type_symbol& type);
    std::size_t get_type_alignment(type_symbol type);
    bytemath::fixed_int_options get_fixed_int_options(type_symbol const& type) const;
    bytemath::fixed_float_options get_fixed_float_options(type_symbol const& type) const;
    char const* fixed_int_instruction_name(fixed_int_instruction instruction) const;
    char const* fixed_float_instruction_name(fixed_float_instruction instruction) const;
    void exec_fixed_int_binary_op(fixed_int_instruction instruction, local_index a_slot, local_index b_slot, local_index result_slot, fixed_int_binary_op op);
    void exec_mut_fixed_int_binary_op(fixed_int_instruction instruction, local_index target_slot, local_index value_slot, fixed_int_binary_op op);
    void exec_fixed_float_binary_op(fixed_float_instruction instruction, local_index a_slot, local_index b_slot, local_index result_slot, fixed_float_binary_op op);
    void exec_mut_fixed_float_binary_op(fixed_float_instruction instruction, local_index target_slot, local_index value_slot, fixed_float_binary_op op);
    void exec_fixed_float_compare_op(char const* instruction_name, local_index a_slot, local_index b_slot, local_index result_slot, fixed_float_compare_op op);
    void exec_mut_bitwise_binary_op(local_index target_slot, local_index value_slot, mut_bitwise_binary_op op, bool invert);

    std::shared_ptr< local > output(local_index slot);

    // If the object is a reference, it returns a pointer to the referenced object,
    // otherwise it returns a pointer to the object itself.
    pointer_impl get_pointer_to(std::size_t frame, local_index slot);

    // make_pointer_to creates pointer to the object.
    // Unlike get_pointer_to, it does not do any special handling for references,
    // so can in principle create a pointer to a reference.
    pointer_impl make_pointer_to(std::shared_ptr< local > object);

    void transition(vmir2::block_index block);
    void transition3(quxlang::vmir2::block_index block);
    bool transition_normal_exit();

    void exec_instr_val(vmir2::increment const& inc);

    void begin_lifetime(std::shared_ptr< local > object);
    void end_lifetime(std::shared_ptr< local > object);

    std::shared_ptr< local > output_local(local_index at);
    /** Returns the unique constexpr storage object associated with a global symbol, creating it from the target slot type on first use. */
    std::shared_ptr< local > get_or_create_global_storage(type_symbol symbol, type_symbol storage_type);
    /** Ensures an antestatal global has a top-level object root for pointer identity. */
    void ensure_antestatal_global_object(type_symbol symbol, std::shared_ptr< local > const& storage_local, type_symbol storage_type);
    /** Returns the unique constexpr initguard object associated with a symbol, creating it on first use. */
    std::shared_ptr< local > get_or_create_initguard(type_symbol symbol);
    /** Decodes the current initguard state from the guard object's data payload. */
    initguard_state get_initguard_state(std::shared_ptr< local > const& guard);

    /** Stores the requested initguard state into the guard object's one-byte payload. */
    void set_initguard_state(std::shared_ptr< local > const& guard, initguard_state state);
    /** Initializes a lock token so it refers to the supplied guard during an active acquisition. */
    void set_initguard_lock(std::shared_ptr< local > const& lock, std::shared_ptr< local > const& guard);
    /** Aborts an in-flight acquisition when a live initguard lock unwinds out of scope. */
    void abort_initguard_lock_if_needed(type_symbol slot_type, std::shared_ptr< local > const& lock);
    /** Materializes a reference to the unique constexpr storage backing the requested global symbol. */
    void do_get_global_storage(type_symbol symbol, local_index target_ref);
    /** Materializes a reference to the unique constexpr initguard backing the requested symbol. */
    void do_initguard_global_get_ref(type_symbol symbol, local_index target_ref);
    /** Commits a successful initguard acquisition, transitioning the referenced guard to initialized. */
    void do_initguard_release(local_index lock_slot);
    /** Aborts an active initguard acquisition, returning the referenced guard to the uninitialized state. */
    void do_initguard_abort(local_index lock_slot);
    /** Attempts to acquire a symbol's initguard and branches based on whether initialization is required. */
    void do_initguard_try_acquire(type_symbol symbol, local_index target_lock, block_index target_acquired, block_index target_already_initialized);
    /** Resolves a live storage object from a storage reference local without changing the reference's lifetime. */
    std::shared_ptr< local > get_storage_local_from_reference(local_index storage_ref, std::string const& instruction_name);
    /** Materializes a reference to a precomputed antestatal global object. */
    void do_get_antestatal_ref(type_symbol symbol, local_index target_ref);
    /** Checks whether the referenced storage can contain an object of the requested type. */
    void expect_storage_accepts_type(local_index storage_ref, std::shared_ptr< local > const& storage_local, type_symbol object_type, std::string const& instruction_name);

    void init_local_storage(std::size_t frame_idx, local_index index)
    {
        // assert(!slot_alive(index));

        if (stack.at(frame_idx).local_values[index] == nullptr)
        {
            auto type = stack.at(frame_idx).ir3->local_types[index].type;
            stack.at(frame_idx).local_values[index] = create_object(type);
        }
    }

    std::shared_ptr< local > constdata(std::vector< std::byte > const& data);

    void exec_instr_val_incdec(local_index val, local_index result, bool increment, bool postfix);

    void exec_incdec_int(local_index input_slot, local_index output_slot, bool increment, bool postfix);
    void exec_incdec_ptr(local_index input_slot, local_index output_slot, bool increment, bool postfix);

    void exec_instr_val(vmir2::assert_instr const& asrt);
    void exec_instr_val(vmir2::decrement const& dec);
    void exec_instr_val(vmir2::preincrement const& inc);
    void exec_instr_val(vmir2::predecrement const& dec);
    void exec_instr_val(vmir2::load_const_zero const& lcz);
    void exec_instr_val(vmir2::unimplemented const&);
    void exec_instr_val(vmir2::load_const_bool const& lcb);
    void exec_instr_val(vmir2::access_field const& acf);
    void exec_instr_val(vmir2::swap const& swp);
    void exec_instr_val(vmir2::to_bool const& lcz);
    void exec_instr_val(vmir2::to_bool_not const& acf);
    void exec_instr_val(vmir2::access_array const& aca);
    void exec_instr_val(vmir2::access_pointer const& acp);
    void exec_instr_val(vmir2::invoke const& inv);
    void exec_instr_val(vmir2::invoke_indirect const& inv);
    void exec_instr_val(vmir2::interface_init const& inv);
    void exec_instr_val(vmir2::interface_invoke const& inv);
    void exec_instr_val(vmir2::interface_is_default const& inv);
    void exec_instr_val(vmir2::get_procedure_ptr const& gpp);
    void exec_instr_val(vmir2::make_reference const& mrf);
    void exec_instr_val(vmir2::jump const& jmp);
    void exec_instr_val(vmir2::branch const& brn);
    void exec_instr_val(vmir2::runtime_constexpr const& rce);
    void exec_instr_val(vmir2::initguard_try_acquire const& ita);
    void exec_instr_val(vmir2::cast_ptrref const& cst);
    void exec_instr_val(vmir2::constexpr_set_result const& csr);
    void exec_instr_val(vmir2::constexpr_set_result2 const& csr);
    void exec_instr_val(vmir2::constexpr_make_proxy const& cmp);
    void exec_instr_val(vmir2::constexpr_output_byte const& cob);
    void exec_instr_val(vmir2::load_const_value const& lcv);
    void exec_instr_val(vmir2::canonicalize_float const& cpf);
    void exec_instr_val(vmir2::get_value_byte const& gvb);
    void exec_instr_val(vmir2::set_value_byte const& svb);
    void exec_instr_val(vmir2::make_pointer_to const& mpt);
    void exec_instr_val(vmir2::storage_init const& sin);
    void exec_instr_val(vmir2::storage_init_start const& sis);
    void exec_instr_val(vmir2::storage_deinit_start const& sds);
    void exec_instr_val(vmir2::storage_pun const& spn);
    void exec_instr_val(vmir2::constexpr_alloc const& cal);
    void exec_instr_val(vmir2::constexpr_alloc_multiple const& cal);
    void exec_instr_val(vmir2::constexpr_dealloc const& cal);
    void exec_instr_val(vmir2::constexpr_dealloc_multiple const& cal);
    void exec_instr_val(vmir2::get_global_storage const& ggs);
    void exec_instr_val(vmir2::get_antestatal_ref const& gar);
    void exec_instr_val(vmir2::initguard_global_get_ref const& igr);
    void exec_instr_val(vmir2::initguard_release const& igr);
    void exec_instr_val(vmir2::initguard_abort const& iga);
    void exec_instr_val(vmir2::dereference_pointer const& drp);
    void exec_instr_val(vmir2::load_from_ref const& lfr);
    void exec_instr_val(vmir2::ret const& ret);
    void exec_instr_val(vmir2::int_add const& add);
    void exec_instr_val(vmir2::iconv const& icv);
    void exec_instr_val(vmir2::int_sub const& sub);
    void exec_instr_val(vmir2::int_mul const& mul);
    void exec_instr_val(vmir2::int_div const& div);
    void exec_instr_val(vmir2::int_mod const& mod);
    void exec_instr_val(vmir2::mut_int_add const& op);
    void exec_instr_val(vmir2::mut_int_sub const& op);
    void exec_instr_val(vmir2::mut_int_mul const& op);
    void exec_instr_val(vmir2::mut_int_div const& op);
    void exec_instr_val(vmir2::mut_int_mod const& op);
    void exec_instr_val(vmir2::float_add const& op);
    void exec_instr_val(vmir2::float_sub const& op);
    void exec_instr_val(vmir2::float_mul const& op);
    void exec_instr_val(vmir2::float_div const& op);
    void exec_instr_val(vmir2::mut_float_add const& op);
    void exec_instr_val(vmir2::mut_float_sub const& op);
    void exec_instr_val(vmir2::mut_float_mul const& op);
    void exec_instr_val(vmir2::mut_float_div const& op);
    void exec_instr_val(vmir2::float_from_int const& op);
    void exec_instr_val(vmir2::store_to_ref const& str);
    void exec_instr_val(vmir2::load_const_int const& lci);
    void exec_instr_val(vmir2::load_const_float const& lcf);
    void exec_instr_val(vmir2::cmp_eq const& ceq);
    void exec_instr_val(vmir2::cmp_ne const& cne);
    void exec_instr_val(vmir2::cmp_lt const& clt);
    void exec_instr_val(vmir2::cmp_ge const& cge);
    void exec_instr_val(vmir2::float_ieee_eq const& op);
    void exec_instr_val(vmir2::float_ieee_ne const& op);
    void exec_instr_val(vmir2::float_ieee_lt const& op);
    void exec_instr_val(vmir2::float_ieee_gt const& op);
    void exec_instr_val(vmir2::pcmp_eq const& ceq);
    void exec_instr_val(vmir2::pcmp_ne const& cne);
    void exec_instr_val(vmir2::pcmp_lt const& clt);
    void exec_instr_val(vmir2::pcmp_ge const& cge);
    void exec_instr_val(vmir2::gcmp_eq const& ceq);
    void exec_instr_val(vmir2::gcmp_ne const& cne);
    void exec_instr_val(vmir2::gcmp_lt const& clt);
    void exec_instr_val(vmir2::gcmp_ge const& cge);

    // Bitwise operations
    void exec_instr_val(vmir2::bitwise_and const& op);
    void exec_instr_val(vmir2::bitwise_or const& op);
    void exec_instr_val(vmir2::bitwise_xor const& op);
    void exec_instr_val(vmir2::bitwise_nand const& op);
    void exec_instr_val(vmir2::bitwise_nor const& op);
    void exec_instr_val(vmir2::bitwise_nxor const& op);
    void exec_instr_val(vmir2::bitwise_implies const& op);
    void exec_instr_val(vmir2::bitwise_implied const& op);
    void exec_instr_val(vmir2::bitwise_shift_up const& op);
    void exec_instr_val(vmir2::bitwise_shift_down const& op);
    void exec_instr_val(vmir2::bitwise_rotate_up const& op);
    void exec_instr_val(vmir2::bitwise_rotate_down const& op);
    void exec_instr_val(vmir2::bitwise_inverse const& op);
    void exec_instr_val(vmir2::mut_bitwise_and const& op);
    void exec_instr_val(vmir2::mut_bitwise_or const& op);
    void exec_instr_val(vmir2::mut_bitwise_xor const& op);
    void exec_instr_val(vmir2::mut_bitwise_nand const& op);
    void exec_instr_val(vmir2::mut_bitwise_nor const& op);
    void exec_instr_val(vmir2::mut_bitwise_nxor const& op);
    void exec_instr_val(vmir2::mut_bitwise_implies const& op);
    void exec_instr_val(vmir2::mut_bitwise_implied const& op);
    void exec_instr_val(vmir2::mut_bitwise_shift_up const& op);
    void exec_instr_val(vmir2::mut_bitwise_shift_down const& op);
    void exec_instr_val(vmir2::mut_bitwise_rotate_up const& op);
    void exec_instr_val(vmir2::mut_bitwise_rotate_down const& op);
    void exec_instr_val(vmir2::defer_nontrivial_dtor const& dntd);
    void exec_instr_val(vmir2::struct_init_start const& sdn);
    void exec_instr_val(vmir2::struct_init_finish const& scn);
    void exec_instr_val(vmir2::copy_reference const& cpr);
    void exec_instr_val(vmir2::destroy const& dst);
    void exec_instr_val(vmir2::end_lifetime const& elt);
    void exec_instr_val(vmir2::pointer_arith const& par);
    void exec_instr_val(vmir2::pointer_diff const& pdf);
    void exec_instr_val(vmir2::array_init_start const& ais);
    void exec_instr_val(vmir2::array_init_element const& aie);
    void exec_instr_val(vmir2::array_init_index const& air);
    void exec_instr_val(vmir2::array_init_more const& aim);
    void exec_instr_val(vmir2::array_init_finish const& aif);

    std::shared_ptr< local > create_local_value(vmir2::local_index local_idx, bool set_alive);
    std::shared_ptr< local > create_object_skeleton(type_symbol type);

    std::shared_ptr< local > create_object(type_symbol type);

    void init_storage(std::shared_ptr< local > local_value, type_symbol type);
    void begin_lifetime_tree(std::shared_ptr< local > const& object);
    void set_readonly_tree(std::shared_ptr< local > const& object);
    bool has_constexpr_antestatal_global(type_symbol const& symbol) const;
    void mark_missing_antestatal_global(type_symbol const& symbol);
    void collect_missing_antestatal_globals(antestatal_value const& value, std::optional< type_symbol > type = std::nullopt);
    void collect_missing_antestatal_globals(antestatal_access const& access, std::optional< type_symbol > type = std::nullopt);

    std::shared_ptr< local > load_from_reference(local_index local_idx, bool consume);
    pointer_impl load_as_pointer(local_index slot, bool consume);
    void store_as_reference(local_index slot, std::shared_ptr< local > value);
    void store_as_pointer(local_index slot, pointer_impl value);
    type_symbol procedure_symbol(type_symbol routine, std::string calling_convention) const;
    std::shared_ptr< local > get_or_create_procedure(type_symbol routine, std::string calling_convention);
    std::shared_ptr< local > get_or_create_antestatal_global(type_symbol symbol, std::optional< type_symbol > expected_type = std::nullopt);
    void initialize_local_from_antestatal_value(std::shared_ptr< local > const& object, type_symbol type, antestatal_value const& value);
    pointer_impl pointer_from_antestatal_access(antestatal_access const& access, ptrref_type const& ptr_type);
    std::shared_ptr< local > local_from_antestatal_access(antestatal_access const& access);
    antestatal_value materialize_antestatal_value(std::shared_ptr< local > const& object, type_symbol type);
    antestatal_access materialize_antestatal_access(pointer_impl ptr, ptrref_type const& ptr_type);
    std::optional< antestatal_access > access_to_antestatal_local(std::shared_ptr< local > const& target);
    std::optional< antestatal_access > access_to_antestatal_subobject(std::shared_ptr< local > const& object, std::shared_ptr< local > const& target, antestatal_access access);

    bool is_reference_type(quxlang::vmir2::local_index slot);
    bool is_pointer_type(quxlang::vmir2::local_index slot);

    std::vector< std::byte > use_data(local_index slot);

    std::vector< std::byte > consume_local_as_data(vmir2::local_index slot);
    std::vector< std::byte > copy_data(vmir2::local_index slot);

    std::vector< std::byte > local_consume_data(std::shared_ptr< local > local_value);
    void local_set_data(std::shared_ptr< local > local_value, std::vector< std::byte > data);

    bool pointer_is_nullptr(pointer_impl ptr);
    bool pointer_is_one_past_the_end(pointer_impl ptr);
    bool pointer_invalidated(pointer_impl ptr);
    bool pointer_targets_object(pointer_impl ptr);
    std::optional< std::weak_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > > pointer_memberof(pointer_impl ptr);
    std::int64_t pointer_offset_in_array(std::shared_ptr< local > array, pointer_impl ptr);
    pointer_impl pointer_arith(pointer_impl input, std::int64_t offset, type_symbol type);
    std::partial_ordering pointer_compare(pointer_impl a, pointer_impl b);
    constexpr_allocation& register_constexpr_allocation(constexpr_allocation allocation);
    constexpr_allocation& allocation_for_pointer(pointer_impl const& ptr);
    void invalidate_local_tree(std::shared_ptr< local > const& object);
    void ensure_allocation_storage_can_be_freed(constexpr_allocation& allocation);
    void do_constexpr_dealloc(type_symbol storage_type, pointer_impl ptr, std::optional< std::uint64_t > count);

    std::uint64_t consume_u64(local_index slot);
    std::shared_ptr< local > consume_local(local_index slot);
    interface_object load_interface_object(local_index slot, bool consume);

    uint64_t alloc_object_id();
    void set_data(local_index slot, std::vector< std::byte > data);

    bool consume_bool(local_index slot);
    void output_bool(local_index slot_index, bool value);

    slot_state get_current_slot_state(std::size_t frame_index, local_index slot_index);
    quxlang::vmir2::state_map get_expected_state_map_preexec(std::size_t frame_index, std::size_t block_index, std::size_t instruction_index);
    quxlang::vmir2::state_map get_expected_state_map_preexec3(std::size_t frame_index, std::size_t block_index, std::size_t instruction_index);
    quxlang::vmir2::state_map get_current_state_map(std::size_t frame_index);
    quxlang::vmir2::state_map get_current_state_map3(std::size_t frame_index);
    slot_state get_expected_slot_state_postexec(std::size_t frame_index, std::size_t slot_index, std::size_t instruction_index);

    void require_valid_input_precondition(local_index slot);
    void require_valid_output_precondition(local_index slot);

    type_symbol frame_slot_data_type(vmir2::local_index slot);

    std::size_t type_size(type_symbol type);

    local_index get_index(std::size_t frame, std::shared_ptr< local > local_value);
};

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::call_func(cow< type_symbol > functype, vmir2::invocation_args args)
{

    // To call a function we push a stack frame onto the stack, copy any relevant arguments, then return.
    // Actual execution will happen in subsequent calls to exec/exec_instr

    auto type = functype.read();

    std::string funcname_str = quxlang::to_string(type);

    stack.emplace_back();
    stack.back().type = functype;

    stack.back().ir3 = functanoids3.at(functype);
    stack.back().address = {functype, block_index(0), 0};

    if (functype == void_type{})
    {
        assert(args.size() == 0);
        // void_type is a special case when we are executing a top level expression
        // in constexpr context. For example, to evaluate an expression like
        // `a + b < 5`, we create a constexpr "function" which is named "VOID".
        // VOID is not actually a callable function normally, but exists for this
        // instance of the constexpr execution.
        return;
    }

    // The rest of call_func just handles arguments, so if the function just takes no arguments, we can return now.
    if (args.size() == 0)
    {
        return;
    }

    std::string funcname = quxlang::to_string(functype.get());

    cow< vmir2::functanoid_routine3 > current_func_ir_v = stack.back().ir3;

    // Now there should be at minimum 2 frames, the caller frame and the callee frame.
    if (stack.size() < 2)
    {
        throw compiler_bug("Attempt to call function with arguments without a stack frame");
    }

    auto previous_func_ir = stack[stack.size() - 2].ir3;

    std::size_t arg_count = 0;
    std::size_t positional_arg_id = 0;

    // previous is the caller frame, current is the callee frame
    auto& current_frame = stack.back();
    auto& previous_frame = stack[stack.size() - 2];
    auto previous_frame_idx = stack.size() - 2;
    auto current_frame_idx = stack.size() - 1;

    auto const& current_func_ir = current_func_ir_v.get();

    auto handle_arg = [&](vmir2::local_index previous_arg_index, vmir2::local_index new_arg_index, rpnx::variant< std::size_t, std::string > param_index)
    {
        type_symbol param_type;
        if (param_index.template type_is< std::size_t >())
        {
            param_type = current_func_ir.parameters.positional.at(param_index.get_as< std::size_t >()).type;
        }
        else
        {
            param_type = current_func_ir.parameters.named.at(param_index.get_as< std::string >()).type;
        }

        std::string param_type_str = quxlang::to_string(param_type);

        if (!typeis< nvalue_slot >(param_type))
        {
            if (!(previous_frame.local_values[previous_arg_index] && previous_frame.local_values[previous_arg_index]->alive()))
            {
                throw constexpr_logic_execution_error("Error in argument passing: argument is not alive");
            }
        }

        if (typeis< nvalue_slot >(param_type) || typeis< dvalue_slot >(param_type))
        {
            // Slots create a shared reference to the same object in a previous frame, unlike all other
            // calls which consume their arguments.
            //
            // That means NEW[T] / DESTROY[T] parameters operate on the caller's existing storage
            // rather than receiving a copied value.

            if (typeis< nvalue_slot >(param_type))
            {
                // The existing storage might not exist, allocate it now if so.

                if (!previous_frame.slot_has_storage(previous_arg_index))
                {
                    init_local_storage(previous_frame_idx, previous_arg_index);
                }
            }
            else
            {
                assert(previous_frame.slot_alive(previous_arg_index));
            }

            assert(previous_frame.slot_has_storage(previous_arg_index));

            current_frame.local_values[new_arg_index] = previous_frame.local_values[previous_arg_index];

            if (typeis< dvalue_slot >(param_type))
            {
                // DESTROY[T] consumes the caller-side object immediately.
                //
                // The owning STORAGE(...) slot retains the storage metadata; the destroy delegate
                // itself is just a temporary alias and can disappear completely from the caller frame.
                previous_frame.local_values[previous_arg_index] = nullptr;
            }

            // The caller should have already initialized the local, not here.
            assert(current_frame.slot_has_storage(new_arg_index));

            if (typeis< nvalue_slot >(param_type))
            {
                // NEW& values should NOT be alive when we enter the function,
                // and are alive when we return except via exception
                assert(!current_frame.local_values[new_arg_index]->alive());
            }

            if (typeis< dvalue_slot >(param_type))
            {
                // DESTROY& values should be alive when we enter the function
                // and are not alive when we return OR throw an exception
                assert(current_frame.local_values[new_arg_index]->alive());
            }
        }

        else
        {
            assert(!typeis< nvalue_slot >(param_type) && !typeis< dvalue_slot >(param_type));
            // In all other cases, the argument is consumed by the call,

            current_frame.local_values[new_arg_index] = previous_frame.local_values[previous_arg_index];
            previous_frame.local_values[previous_arg_index] = nullptr;

            // TODO: If the type is trivially relocatable, invalidate all prior references to it now
            // We don't currently pass this information, so this is a TODO.
        }
        arg_count++;
    };

    // Look through all slots in the new function for arguments
    for (std::size_t param_index(0); param_index < current_func_ir.parameters.positional.size(); param_index++)
    {
        auto const& positional_param = current_func_ir.parameters.positional[param_index];
        local_index new_arg_index = positional_param.local_index;
        auto const& slot = current_func_ir.local_types[new_arg_index];

        if (positional_arg_id >= args.positional.size())
        {
            throw compiler_bug("Missing positional argument");
        }

        auto previous_arg_index = args.positional.at(positional_arg_id);

        positional_arg_id++;

        handle_arg(previous_arg_index, new_arg_index, param_index);
    }

    for (auto const& [name, named_param] : current_func_ir.parameters.named)
    {
        if (!args.named.contains(name))
        {
            throw compiler_bug("Missing named argument: " + name);
        }

        auto previous_arg_index = args.named.at(name);
        auto new_arg_index = named_param.local_index;

        handle_arg(previous_arg_index, new_arg_index, name);
    }

    // We should have gone through all args at this point.
    assert(arg_count == args.size());
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec()
{
    // print all functanoids

    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        for (auto const& func : functanoids3)
        {



            quxlang::vmir2::assembler ir_printer(func.second.get());
            ir_printer.source_index = this->printer_source_index;
            std::cout << "Functanoid: " << quxlang::to_string(*(func.first)) << std::endl;
            std::cout << ir_printer.to_string(func.second.get()) << std::endl;
        }
    }


    while (!stack.empty())
    {

        exec_instr3();
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec3()
{
    exec_mode = 2;
    exec();
}
quxlang::vmir2::state_diff quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_state_diff()
{
    auto const& current_instr_address = stack.back().address;
    state_map expected_state;

    expected_state = get_expected_state_map_preexec3(current_frame_index(), current_instr_address.block, current_instr_address.instruction_index);

    state_map current_statemap;

    current_statemap = get_current_state_map3(current_frame_index());

    state_diff result;

    for (auto const& [index, state] : current_statemap)
    {
        if (expected_state[index] != state)
        {
            auto estate = expected_state[index];
            result[index] = {state, expected_state[index]};
        }
    }

    return result;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr()
{
    throw compiler_bug("removed");
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr3()
{
    interp_addr& current_instr_address = stack.back().address;

    auto const& current_func = stack.back().type;

    auto const & current_func_ir = functanoids3.at(current_func.get());

    auto const& current_block = current_func_ir->blocks.at(current_instr_address.block);

    std::optional<quxlang::vmir2::assembler> ir_printer;
    if constexpr(QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        ir_printer.emplace(current_func_ir.get());
        ir_printer->source_index = this->printer_source_index;
    }

    if (current_instr_address.instruction_index < current_block.instructions.size())
    {
        vm_instruction const& instr = current_block.instructions.at(current_instr_address.instruction_index);

        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::cout << "Executing in constexpr " << quxlang::to_string(current_func.get()) << " block " << current_instr_address.block << " instruction " << current_instr_address.instruction_index << ": " << ir_printer->to_string(instr) << std::endl;
        }
        //  If there is an error here, it usually means there is an instruction which is not implemented
        //  on the constexpr virtual machine. Instructions which are illegal in a constexpr context
        //  should be implemented to throw a derivative of std::logic_error.

        if constexpr (QUXLANG_IN_DEBUG)
        {
            auto expected_state = get_expected_state_map_preexec3(current_frame_index(), current_instr_address.block, current_instr_address.instruction_index);
            auto start_frame_id = current_frame_index();
            auto current_statemap = get_current_state_map3(current_frame_index());

            auto state_diff = this->get_state_diff();

            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                if (current_statemap != expected_state)
                {
                    std::cout << " - Expected before state: " << ir_printer->to_string(expected_state) << std::endl;
                    std::cout << " - Actual before state: " << ir_printer->to_string(current_statemap) << std::endl;
                    for (auto const& [index, states] : state_diff)
                    {
                        std::cout << "   - Slot " << index << " state mismatch: expected " << ir_printer->to_string(index, states.second) << ", got " << ir_printer->to_string(index, states.first) << std::endl;
                    }
                }
                assert(current_statemap == expected_state);

            }

        }

        std::size_t stack_size1 = stack.size();
        rpnx::apply_visitor< void >(instr,
                                    [this](auto const& param)
                                    {
                                        return this->exec_instr_val(param);
                                    });
        std::size_t stack_size2 = stack.size();
        current_instr_address.instruction_index++;


        return;
    }

    auto const terminator_instruction = current_block.terminator;
    if (!terminator_instruction)
    {
        throw constexpr_logic_execution_error("Constexpr execution reached end of block with undefined behavior");
    }

    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        std::cout << "Executing in constexpr " << quxlang::to_string(current_func.get()) << " block " << current_instr_address.block << " terminator " << current_instr_address.instruction_index << ": " << ir_printer->to_string(terminator_instruction.value()) << std::endl;
    }

    rpnx::apply_visitor< void >(*terminator_instruction,
                                [this](auto const& param)
                                {
                                    return this->exec_instr_val(param);
                                });
    return;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::raise_fault(std::string const& fault_name)
{
    // Currently, faults are not permitted during constexpr execution.
    // Centalizing fault handling in one place in-case we extend the runtime to allow them during constexpr.
    throw constexpr_logic_execution_error("During constexpr execution: Runtime fault: " + fault_name);
}
quxlang::type_symbol quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_local_type(local_index slot)
{
    return stack.back().ir3->local_types.at(slot).type;
}
quxlang::type_symbol quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_local_type(std::size_t frame, local_index slot)
{
    return stack[frame].ir3->local_types.at(slot).type;
}

std::size_t quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_type_size(const type_symbol& type)
{
    auto expr_u64 = [](expression const& expr) -> std::uint64_t
    {
        if (!expr.type_is< expression_numeric_literal >())
        {
            throw std::logic_error("Expected numeric literal in storage type");
        }
        return parsers::str_to_int< std::uint64_t >(expr.get_as< expression_numeric_literal >().value);
    };

    std::string type_str = quxlang::to_string(type);

    if (typeis< attached_type_reference >(type))
    {
        attached_type_reference const& attached = as< attached_type_reference >(type);
        if (typeis< void_type >(attached.carrying_type))
        {
            return 0;
        }
        return get_type_size(attached.carrying_type);
    }

    if (typeis< int_type >(type))
    {
        return (type.get_as< int_type >().bits + 7) / 8;
    }

    if (typeis< float_type >(type))
    {
        return (type.get_as< float_type >().bits + 7) / 8;
    }

    if (typeis< byte_type >(type))
    {
        return 1;
    }

    if (typeis< initguard_type >(type) || typeis< initguard_lock_type >(type))
    {
        return 8;
    }

    if (typeis< constexpr_proxy >(type))
    {
        return 0;
    }

    if (typeis< bool_type >(type))
    {
        return 1;
    }

    if (typeis< storage >(type))
    {
        std::size_t max_size = 0;
        for (auto const& stored_type : as< storage >(type).storable_types)
        {
            max_size = std::max(max_size, get_type_size(stored_type));
        }
        return max_size;
    }

    if (typeis< aligned_storage >(type))
    {
        return expr_u64(as< aligned_storage >(type).size);
    }

    // Pointer data is stored in ref, not data bytes.
    if (typeis< ptrref_type >(type) || typeis< procedure_type >(type))
    {
        return 0;
    }

    // Structs should not have data bytes
    if (this->class_layouts.contains(type))
    {
        return 0;
    }

    // TODO: Consider if structs should have "size" during constexpr evaluation

    return 0;
}

std::size_t quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_type_alignment(type_symbol type)
{
    auto expr_u64 = [](expression const& expr) -> std::uint64_t
    {
        if (!expr.type_is< expression_numeric_literal >())
        {
            throw std::logic_error("Expected numeric literal in storage type");
        }
        return parsers::str_to_int< std::uint64_t >(expr.get_as< expression_numeric_literal >().value);
    };

    if (typeis< attached_type_reference >(type))
    {
        attached_type_reference const& attached = as< attached_type_reference >(type);
        if (typeis< void_type >(attached.carrying_type))
        {
            return 1;
        }
        return get_type_alignment(attached.carrying_type);
    }
    if (typeis< int_type >(type))
    {
        auto sz = get_type_size(type);
        return std::min< std::size_t >(sz, 8);
    }
    if (typeis< float_type >(type))
    {
        auto sz = get_type_size(type);
        return std::min< std::size_t >(sz, 8);
    }
    if (typeis< initguard_type >(type) || typeis< initguard_lock_type >(type))
    {
        return 8;
    }
    if (typeis< byte_type >(type) || typeis< bool_type >(type))
    {
        return 1;
    }
    if (typeis< ptrref_type >(type) || typeis< procedure_type >(type))
    {
        return 1;
    }
    if (typeis< constexpr_proxy >(type))
    {
        return 1;
    }
    if (class_layouts.contains(type))
    {
        return class_layouts.at(type).align;
    }
    if (typeis< storage >(type))
    {
        std::size_t max_align = 1;
        for (auto const& stored_type : as< storage >(type).storable_types)
        {
            max_align = std::max(max_align, get_type_alignment(stored_type));
        }
        return max_align;
    }
    if (typeis< aligned_storage >(type))
    {
        return expr_u64(as< aligned_storage >(type).align);
    }
    return 1;
}

quxlang::bytemath::fixed_int_options quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_fixed_int_options(type_symbol const& type) const
{
    bytemath::fixed_int_options opts{};

    if (type.type_is< byte_type >())
    {
        opts.bits = 8;
        opts.has_sign = false;
        opts.overflow_undefined = false;
        return opts;
    }

    if (!type.type_is< int_type >())
    {
        throw std::runtime_error("expected primitive integer type");
    }

    int_type const& int_type_info = type.get_as< int_type >();
    opts.bits = int_type_info.bits;
    opts.has_sign = int_type_info.has_sign;
    opts.overflow_undefined = false;
    return opts;
}

quxlang::bytemath::fixed_float_options quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_fixed_float_options(type_symbol const& type) const
{
    if (!type.type_is< float_type >())
    {
        throw std::runtime_error("expected primitive floating point type");
    }

    auto const& float_type_info = type.get_as< float_type >();
    return bytemath::fixed_float_options{.bits = float_type_info.bits, .exponent_bits = float_type_info.exponent_bits};
}

char const* quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::fixed_int_instruction_name(fixed_int_instruction instruction) const
{
    switch (instruction)
    {
    case fixed_int_instruction::add:
        return "IADD";
    case fixed_int_instruction::sub:
        return "ISUB";
    case fixed_int_instruction::mul:
        return "IMUL";
    case fixed_int_instruction::div:
        return "IDIV";
    case fixed_int_instruction::mod:
        return "IMOD";
    }

    throw compiler_bug("unknown fixed integer instruction");
}

char const* quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::fixed_float_instruction_name(fixed_float_instruction instruction) const
{
    switch (instruction)
    {
    case fixed_float_instruction::add:
        return "FADD";
    case fixed_float_instruction::sub:
        return "FSUB";
    case fixed_float_instruction::mul:
        return "FMUL";
    case fixed_float_instruction::div:
        return "FDIV";
    }

    throw compiler_bug("unknown fixed floating point instruction");
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_fixed_int_binary_op(fixed_int_instruction instruction, local_index a_slot, local_index b_slot, local_index result_slot, fixed_int_binary_op op)
{
    char const* instruction_name = fixed_int_instruction_name(instruction);

    require_valid_input_precondition(a_slot);
    require_valid_input_precondition(b_slot);
    require_valid_output_precondition(result_slot);

    std::shared_ptr< local > a_local = consume_local(a_slot);
    std::shared_ptr< local > b_local = consume_local(b_slot);
    std::shared_ptr< local > result_local = output_local(result_slot);

    auto& a_data = a_local->data;
    auto& b_data = b_local->data;
    auto& result_data = result_local->data;

    if (a_data.size() != b_data.size())
    {
        throw std::runtime_error(std::string(instruction_name) + ": operands have different sizes");
    }

    type_symbol a_type = get_local_type(a_slot);
    type_symbol b_type = get_local_type(b_slot);
    type_symbol result_type = get_local_type(result_slot);

    if (a_type != b_type || a_type != result_type)
    {
        throw std::runtime_error(std::string(instruction_name) + ": type mismatch among operands");
    }

    bytemath::fixed_int_options opts = get_fixed_int_options(a_type);
    std::size_t expected_size = (opts.bits + 7) / 8;

    if (a_data.size() != expected_size)
    {
        throw std::runtime_error(std::string(instruction_name) + ": operand size does not match type width");
    }

    result_data.resize(expected_size, std::byte{0});

    bytemath::int_result res = op(opts, a_data, b_data);
    if (res.result_is_undefined)
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": undefined behavior");
    }

    if (res.data_bytes.size() != expected_size)
    {
        throw compiler_bug(std::string(instruction_name) + ": bytemath returned unexpected result size");
    }

    result_data = std::move(res.data_bytes);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_mut_fixed_int_binary_op(fixed_int_instruction instruction, local_index target_slot, local_index value_slot, fixed_int_binary_op op)
{
    char const* instruction_name = fixed_int_instruction_name(instruction);

    require_valid_input_precondition(target_slot);
    require_valid_input_precondition(value_slot);

    std::shared_ptr< local > target_ref = consume_local(target_slot);
    std::shared_ptr< local > value_local = consume_local(value_slot);
    if (!target_ref->ref.has_value())
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": target is not a reference");
    }
    if (pointer_invalidated(*target_ref->ref))
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": target reference is invalid");
    }

    auto target_value = target_ref->ref->pointer_target.value().lock();
    if (!target_value)
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": target reference is invalid");
    }

    auto& target_data = target_value->data;
    auto& value_data = value_local->data;

    if (target_data.size() != value_data.size())
    {
        throw std::runtime_error(std::string(instruction_name) + ": operands have different sizes");
    }

    type_symbol target_type = remove_ref(get_local_type(target_slot));
    type_symbol value_type = get_local_type(value_slot);
    if (target_type != value_type)
    {
        throw std::runtime_error(std::string(instruction_name) + ": type mismatch among operands");
    }

    bytemath::fixed_int_options opts = get_fixed_int_options(target_type);
    std::size_t expected_size = (opts.bits + 7) / 8;
    if (target_data.size() != expected_size)
    {
        throw std::runtime_error(std::string(instruction_name) + ": operand size does not match type width");
    }

    bytemath::int_result res = op(opts, target_data, value_data);
    if (res.result_is_undefined)
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": undefined behavior");
    }
    if (res.data_bytes.size() != expected_size)
    {
        throw compiler_bug(std::string(instruction_name) + ": bytemath returned unexpected result size");
    }

    local_set_data(target_value, std::move(res.data_bytes));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_fixed_float_binary_op(fixed_float_instruction instruction, local_index a_slot, local_index b_slot, local_index result_slot, fixed_float_binary_op op)
{
    char const* instruction_name = fixed_float_instruction_name(instruction);

    require_valid_input_precondition(a_slot);
    require_valid_input_precondition(b_slot);
    require_valid_output_precondition(result_slot);

    std::shared_ptr< local > a_local = consume_local(a_slot);
    std::shared_ptr< local > b_local = consume_local(b_slot);
    std::shared_ptr< local > result_local = output_local(result_slot);

    auto& a_data = a_local->data;
    auto& b_data = b_local->data;
    auto& result_data = result_local->data;

    if (a_data.size() != b_data.size())
    {
        throw std::runtime_error(std::string(instruction_name) + ": operands have different sizes");
    }

    type_symbol a_type = get_local_type(a_slot);
    type_symbol b_type = get_local_type(b_slot);
    type_symbol result_type = get_local_type(result_slot);

    if (a_type != b_type || a_type != result_type)
    {
        throw std::runtime_error(std::string(instruction_name) + ": type mismatch among operands");
    }

    bytemath::fixed_float_options opts = get_fixed_float_options(a_type);
    std::size_t expected_size = (opts.bits + 7) / 8;

    if (a_data.size() != expected_size)
    {
        throw std::runtime_error(std::string(instruction_name) + ": operand size does not match type width");
    }

    result_data.resize(expected_size, std::byte{0});

    bytemath::float_result res = op(opts, a_data, b_data);
    if (res.result_is_undefined)
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": undefined behavior");
    }

    if (res.data_bytes.size() != expected_size)
    {
        throw compiler_bug(std::string(instruction_name) + ": bytemath returned unexpected result size");
    }

    result_data = std::move(res.data_bytes);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_mut_fixed_float_binary_op(fixed_float_instruction instruction, local_index target_slot, local_index value_slot, fixed_float_binary_op op)
{
    char const* instruction_name = fixed_float_instruction_name(instruction);

    require_valid_input_precondition(target_slot);
    require_valid_input_precondition(value_slot);

    std::shared_ptr< local > target_ref = consume_local(target_slot);
    std::shared_ptr< local > value_local = consume_local(value_slot);
    if (!target_ref->ref.has_value())
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": target is not a reference");
    }
    if (pointer_invalidated(*target_ref->ref))
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": target reference is invalid");
    }

    auto target_value = target_ref->ref->pointer_target.value().lock();
    if (!target_value)
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": target reference is invalid");
    }

    auto& target_data = target_value->data;
    auto& value_data = value_local->data;

    if (target_data.size() != value_data.size())
    {
        throw std::runtime_error(std::string(instruction_name) + ": operands have different sizes");
    }

    type_symbol target_type = remove_ref(get_local_type(target_slot));
    type_symbol value_type = get_local_type(value_slot);
    if (target_type != value_type)
    {
        throw std::runtime_error(std::string(instruction_name) + ": type mismatch among operands");
    }

    bytemath::fixed_float_options opts = get_fixed_float_options(target_type);
    std::size_t expected_size = (opts.bits + 7) / 8;
    if (target_data.size() != expected_size)
    {
        throw std::runtime_error(std::string(instruction_name) + ": operand size does not match type width");
    }

    bytemath::float_result res = op(opts, target_data, value_data);
    if (res.result_is_undefined)
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": undefined behavior");
    }
    if (res.data_bytes.size() != expected_size)
    {
        throw compiler_bug(std::string(instruction_name) + ": bytemath returned unexpected result size");
    }

    local_set_data(target_value, std::move(res.data_bytes));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_fixed_float_compare_op(char const* instruction_name, local_index a_slot, local_index b_slot, local_index result_slot, fixed_float_compare_op op)
{
    require_valid_input_precondition(a_slot);
    require_valid_input_precondition(b_slot);
    require_valid_output_precondition(result_slot);

    auto a_type = get_local_type(a_slot);
    auto b_type = get_local_type(b_slot);
    if (a_type != b_type || !a_type.type_is< float_type >())
    {
        throw std::runtime_error(std::string(instruction_name) + ": operands must have the same floating point type");
    }

    auto a_data = consume_local_as_data(a_slot);
    auto b_data = consume_local_as_data(b_slot);
    auto result = op(get_fixed_float_options(a_type), std::move(a_data), std::move(b_data));
    if (result.result_is_undefined)
    {
        throw constexpr_logic_execution_error(std::string("error executing ") + instruction_name + ": undefined behavior");
    }
    set_data(result_slot, {std::byte(result.result ? 1 : 0)});
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::output(local_index slot)
{
    auto& frame = get_current_frame();

    if (frame.local_values[slot] == nullptr)
    {
        auto const& local_type = get_local_type(slot);
        frame.local_values[slot] = create_object(local_type);
    }

    if (frame.local_values[slot]->alive())
    {
        throw compiler_bug("Attempt to output to object which is already existing");
    }

    if (!frame.local_values[slot]->storage_initiated)
    {
        throw compiler_bug("Attempt to output to object which is not storage initialized");
    }

    return frame.local_values[slot];
}

quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_impl quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_pointer_to(std::size_t frame, local_index slot)
{
    // This can either be a value or a reference,

    bool is_ref = quxlang::is_ref(get_local_type(frame, slot));

    if (!is_ref)
    {
        pointer_impl result;
        result.pointer_target = stack[frame].local_values[slot];
        if (result.pointer_target.value().lock() == nullptr)
        {
            throw compiler_bug("Attempt to take reference to non-existant storage location");
        }

        return result;
    }
    else
    {
        auto ptr = stack[frame].local_values[slot]->ref;
        assert(ptr.has_value());
        if (ptr->pointer_target.value().lock() == nullptr)
        {
            throw compiler_bug("Attempt to create pointer to non-extant storage location");
        }
        return *ptr;
    }
}
quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_impl quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::make_pointer_to(std::shared_ptr< local > object)
{
    return pointer_impl{.pointer_target = object};
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::load_const_zero const& lcz)
{
    auto const& type = get_local_type(lcz.target);

    auto sz = get_type_size(type);

    if (is_ref(type))
    {
        throw compiler_bug("Cannot load zero into reference");
    }

    if (get_current_frame().local_values[lcz.target] && get_current_frame().local_values[lcz.target]->alive())
    {
        throw compiler_bug("Local value already has pointer");
    }

    auto local_ptr = output_local(lcz.target);

    local_ptr->data = std::vector< std::byte >(sz, std::byte{0});
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::unimplemented const&)
{
    throw constexpr_logic_execution_error("Unimplemented instruction executed in constexpr interpreter");
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::initguard_global_get_ref const& igr)
{
    do_initguard_global_get_ref(igr.symbol, igr.target_ref);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::do_initguard_global_get_ref(type_symbol symbol, local_index target_ref)
{
    auto guard = get_or_create_initguard(symbol);
    auto guard_ref = output_local(target_ref);
    guard_ref->ref = pointer_impl{.pointer_target = guard};
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::initguard_release const& igr)
{
    do_initguard_release(igr.lock);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::do_initguard_release(local_index lock_slot)
{
    auto lock = consume_local(lock_slot);
    if (!lock)
    {
        throw constexpr_logic_execution_error("INITGUARD_RELEASE on invalid lock");
    }

    if (!lock->ref.has_value() || !lock->ref->pointer_target.has_value())
    {
        throw constexpr_logic_execution_error("INITGUARD_RELEASE on non-lock value");
    }

    auto guard = lock->ref->pointer_target->lock();
    lock->ref = std::nullopt;
    if (!guard)
    {
        throw constexpr_logic_execution_error("INITGUARD_RELEASE on expired guard");
    }

    if (get_initguard_state(guard) != initguard_state::initializing)
    {
        throw constexpr_logic_execution_error("INITGUARD_RELEASE on guard that is not initializing");
    }

    set_initguard_state(guard, initguard_state::initialized);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::initguard_abort const& iga)
{
    do_initguard_abort(iga.lock);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::do_initguard_abort(local_index lock_slot)
{
    auto lock = consume_local(lock_slot);
    if (!lock)
    {
        throw constexpr_logic_execution_error("INITGUARD_ABORT on invalid lock");
    }

    if (!lock->ref.has_value() || !lock->ref->pointer_target.has_value())
    {
        throw constexpr_logic_execution_error("INITGUARD_ABORT on non-lock value");
    }

    auto guard = lock->ref->pointer_target->lock();
    lock->ref = std::nullopt;
    if (!guard)
    {
        throw constexpr_logic_execution_error("INITGUARD_ABORT on expired guard");
    }

    if (get_initguard_state(guard) != initguard_state::initializing)
    {
        throw constexpr_logic_execution_error("INITGUARD_ABORT on guard that is not initializing");
    }

    set_initguard_state(guard, initguard_state::uninitialized);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::load_const_bool const& lcb)
{
    auto const& type = get_local_type(lcb.target);

    auto sz = get_type_size(type);

    if (type != bool_type{})
    {
        throw compiler_bug("Cannot load bool into non-bool type");
    }

    if (get_current_frame().local_values[lcb.target] && get_current_frame().local_values[lcb.target]->alive())
    {
        throw compiler_bug("Local value already has pointer");
    }

    auto local_ptr = output_local(lcb.target);

    if (lcb.value)
    {
        local_ptr->data = std::vector< std::byte >(sz, std::byte{1});
    }
    else
    {
        local_ptr->data = std::vector< std::byte >(sz, std::byte{0});
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::access_field const& acf)
{
    auto& frame = get_current_frame();
    auto parent_ref_slot = consume_local(acf.base_index);

    if (parent_ref_slot == nullptr || !parent_ref_slot->ref.has_value())
    {
        throw compiler_bug("shouldn't be possible");
    }

    if (!parent_ref_slot->ref.value().pointer_target.has_value())
    {
        throw constexpr_logic_execution_error("access field of nullptr is undefined behavior");
    }

    auto ref_to_ptr = parent_ref_slot->ref.value().pointer_target.value().lock();

    if (ref_to_ptr == nullptr)
    {
        // TODO: this isn't a compiler bug but rather a coding bug
        throw compiler_bug("accessing field of object that does not have field");
    }

    auto field = output_local(acf.store_index);
    auto& field_slot = ref_to_ptr->struct_members.at(acf.field_name);

    type_symbol base_type = get_local_type(acf.base_index);
    type_symbol base_object_type = quxlang::remove_ref(base_type);
    auto layout_it = class_layouts.find(base_object_type);
    if (layout_it == class_layouts.end())
    {
        throw compiler_bug("access field of object with no class layout");
    }

    std::optional< type_symbol > field_type;
    for (class_field_info const& field_info : layout_it->second.fields)
    {
        if (field_info.name == acf.field_name)
        {
            field_type = field_info.type;
            break;
        }
    }
    if (!field_type.has_value())
    {
        throw compiler_bug("access field of object that does not have field in layout");
    }
    if (typeis< attached_type_reference >(*field_type))
    {
        attached_type_reference const& attached = as< attached_type_reference >(*field_type);
        if (typeis< void_type >(attached.carrying_type))
        {
            throw compiler_bug("access field emitted for zero-storage attached binding");
        }
        field_type = attached.carrying_type;
    }

    if (typeis< ptrref_type >(*field_type) && as< ptrref_type >(*field_type).ptr_class == pointer_class::ref)
    {
        if (!field_slot->ref.has_value())
        {
            throw compiler_bug("reference-typed field does not hold a reference");
        }
        field->ref = field_slot->ref;
        return;
    }

    field->ref = pointer_impl{.pointer_target = field_slot};
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::swap const& swp)
{
    auto local_a = load_from_reference(swp.a, true);
    auto local_b = load_from_reference(swp.b, true);

    auto& frame = get_current_frame();

    std::swap(local_a->data, local_b->data);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::to_bool const& tb)
{
    // There are two different versions of TB instruction, one which operates on pointer and one which
    // operates on integers.
    // These work the same on real machines, but because we treat pointers specially in constexpr, we
    // need to handle them differently.

    auto typ = frame_slot_data_type(tb.from);

    if (typ.type_is< ptrref_type >() && !(typ.get_as< ptrref_type >().ptr_class == pointer_class::ref))
    {
        auto& local = get_current_frame().local_values[tb.from];

        if (local == nullptr)
        {
            // Maybe invalid instruction? but this should have been caught earlier
            // throw invalid_instruction_transition_error("missing slot");
            throw compiler_bug("slot missing");
        }

        if (!local->alive())
        {
            throw constexpr_logic_execution_error("Error executing <to_bool>: accessing deallocated object");
        }

        bool result = local->ref.has_value();
        consume_local(tb.from);
        output_bool(tb.to, result);
    }
    else
    {
        auto data = consume_local_as_data(tb.from);

        bool result = false;

        for (auto const& byte : data)
        {
            if (byte != std::byte{0})
            {
                result = true;
                break;
            }
        }

        output_bool(tb.to, result);
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::to_bool_not const& tbn)
{
    // There are two different versions of TB instruction, one which operates on pointer and one which
    // operates on integers.
    // These work the same on real machines, but because we treat pointers specially in constexpr, we
    // need to handle them differently.

    auto typ = frame_slot_data_type(tbn.from);

    if (typ.type_is< ptrref_type >() && !(typ.template as< ptrref_type >().ptr_class == pointer_class::ref))
    {
        auto& local = get_current_frame().local_values[tbn.from];

        if (local == nullptr)
        {
            // Maybe invalid instruction? but this should have been caught earlier
            // throw invalid_instruction_transition_error("missing slot");
            throw compiler_bug("slot missing");
        }

        if (!local->alive())
        {
            throw constexpr_logic_execution_error("Error executing <to_bool>: accessing deallocated object");
        }

        bool result = !local->ref.has_value();
        consume_local(tbn.from);
        output_bool(tbn.to, result);
    }
    else
    {
        auto data = consume_local_as_data(tbn.from);
        bool result = false;

        for (auto const& byte : data)
        {
            if (byte != std::byte{0})
            {
                result = true;
                break;
            }
        }

        output_bool(tbn.to, !result);
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::access_array const& aca)
{
    auto& frame = get_current_frame();
    auto& parent_ref_slot = frame.local_values[aca.base_index];

    if (parent_ref_slot == nullptr || !parent_ref_slot->ref.has_value())
    {
        throw compiler_bug("shouldn't be possible");
    }

    auto ref_to_ptr = parent_ref_slot->ref.value().pointer_target.value().lock();

    if (ref_to_ptr == nullptr)
    {
        // TODO: this isn't a compiler bug but rather a coding bug
        throw compiler_bug("accessing field of object that does not have field");
    }

    auto& field = frame.local_values[aca.store_index];
    if (field != nullptr)
    {
        throw compiler_bug("trying to load a field into existing slot");
    }

    create_local_value(aca.store_index, false);

    auto arry_index = consume_u64(aca.index_index);

    if (arry_index >= ref_to_ptr->array_members.size())
    {
        throw constexpr_logic_execution_error("Array index out of bounds in constexpr execution");
    }

    auto& field_slot = ref_to_ptr->array_members.at(arry_index);

    field->ref = pointer_impl{.pointer_target = field_slot};
    begin_lifetime(field);

    parent_ref_slot = nullptr;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::access_pointer const& acp)
{
    auto ptr = load_as_pointer(acp.base_index, true);

    auto const& offset_type = get_local_type(acp.index_index);
    if (!offset_type.type_is< int_type >())
    {
        throw compiler_bug("Error in [access_pointer]: index is not an integer");
    }

    auto offset_val = consume_local_as_data(acp.index_index);
    auto [offset, ok] = bytemath::le_int_fixed_to_unlimited(get_fixed_int_options(offset_type), offset_val).to_int< std::int64_t >();
    if (!ok)
    {
        throw constexpr_logic_execution_error("access_pointer index overflow in constexpr execution");
    }

    auto ref_type = get_local_type(acp.store_index);
    if (!is_ref(ref_type))
    {
        throw compiler_bug("Attempt to index pointer into non-reference type");
    }

    pointer_impl new_ptr = pointer_arith(ptr, offset, ref_type);
    if (!new_ptr.pointer_target.has_value())
    {
        throw constexpr_logic_execution_error("Array pointer index out of bounds in constexpr execution");
    }

    auto& field = get_current_frame().local_values[acp.store_index];
    if (field != nullptr)
    {
        throw compiler_bug("trying to load pointer index into existing slot");
    }

    create_local_value(acp.store_index, false);
    field->ref = new_ptr;
    begin_lifetime(field);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::invoke const& inv)
{
    call_func(inv.what, inv.args);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::invoke_indirect const& inv)
{
    auto callee = load_indirect_callable_symbol(inv.what_index, true);
    call_func(callee, inv.args);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::interface_init const& inv)
{
    auto target = output_local(inv.target);
    target->interface_value = interface_object{
        .interface_type = inv.interface_type,
        .functions = inv.functions,
        .is_default = inv.is_default,
    };
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::interface_invoke const& inv)
{
    interface_object interface_handle = load_interface_object(inv.interface_value, false);
    if (interface_handle.is_default)
    {
        if (!inv.default_function.has_value())
        {
            throw constexpr_logic_execution_error("Interface default value invoked without a default implementation");
        }

        invocation_args default_args = inv.args;
        default_args.named["THIS"] = inv.interface_value;
        call_func(*inv.default_function, std::move(default_args));
        return;
    }

    auto callee = interface_handle.functions.find(inv.slot);
    if (callee == interface_handle.functions.end())
    {
        throw constexpr_logic_execution_error("Interface value invoked without a matching implementation slot");
    }

    consume_local(inv.interface_value);
    call_func(callee->second, inv.args);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::interface_is_default const& inv)
{
    interface_object interface_handle = load_interface_object(inv.interface_value, true);
    output_bool(inv.result, interface_handle.is_default);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::get_procedure_ptr const& gpp)
{
    auto proc_local = get_or_create_procedure(gpp.routine, gpp.calling_convention);
    auto ptrval = output_local(gpp.pointer_index);
    ptrval->ref = pointer_impl{.pointer_target = proc_local};
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::storage_init const& sin)
{
    auto slot_type = get_local_type(sin.storage);
    if (!typeis< storage >(slot_type) && !typeis< aligned_storage >(slot_type))
    {
        throw compiler_bug("storage_init expects a storage-typed slot");
    }

    auto storage_local = output_local(sin.storage);
    storage_local->stored_object = nullptr;
    storage_local->storage_active_type = std::nullopt;
}
std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_storage_local_from_reference(local_index storage_ref, std::string const& instruction_name)
{
    auto& storage_ref_slot = get_current_frame().local_values.at(storage_ref);
    if (!storage_ref_slot || !storage_ref_slot->alive() || !storage_ref_slot->ref.has_value())
    {
        throw constexpr_logic_execution_error(instruction_name + " expects a live storage reference");
    }
    if (pointer_invalidated(storage_ref_slot->ref.value()))
    {
        throw constexpr_logic_execution_error(instruction_name + " on invalid storage");
    }

    auto storage_local = storage_ref_slot->ref.value().pointer_target.value().lock();
    if (!storage_local || !storage_local->alive())
    {
        throw constexpr_logic_execution_error(instruction_name + " on invalid storage");
    }

    return storage_local;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::expect_storage_accepts_type(local_index storage_ref, std::shared_ptr< local > const& storage_local, type_symbol object_type, std::string const& instruction_name)
{
    auto slot_storage_type = remove_ref(get_local_type(storage_ref));
    bool fits = false;
    if (typeis< storage >(slot_storage_type))
    {
        for (auto const& allowed_type : as< storage >(slot_storage_type).storable_types)
        {
            if (allowed_type == object_type)
            {
                fits = true;
                break;
            }
        }
    }
    else if (typeis< aligned_storage >(slot_storage_type))
    {
        fits = get_type_size(object_type) <= storage_local->data.size() && get_type_alignment(object_type) <= storage_local->storage_alignment;
    }

    if (!fits)
    {
        throw constexpr_logic_execution_error(instruction_name + ": object type is not permitted in target storage");
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::storage_init_start const& sis)
{
    auto storage_local = get_storage_local_from_reference(sis.on_storage, "storage init start");
    auto object_type = get_local_type(sis.target_value);

    if (storage_local->storage_active_type.has_value() || storage_local->stored_object != nullptr)
    {
        throw constexpr_logic_execution_error("storage init start on non-empty storage");
    }

    expect_storage_accepts_type(sis.on_storage, storage_local, object_type, "storage init start");

    auto object_local = create_local_value(sis.target_value, false);
    if (object_local->alive())
    {
        throw compiler_bug("storage init delegate should start dead");
    }
    object_local->storage_owner = storage_local;
    object_local->storage_projection_type = object_type;
    object_local->storage_destroy_delegate = false;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::storage_deinit_start const& sds)
{
    auto storage_local = get_storage_local_from_reference(sds.on_storage, "storage deinit start");
    auto object_type = get_local_type(sds.target_value);

    if (!storage_local->storage_active_type.has_value() || storage_local->storage_active_type.value() != object_type)
    {
        throw constexpr_logic_execution_error("storage deinit start type does not match active object");
    }
    if (storage_local->stored_object == nullptr || !storage_local->stored_object->alive())
    {
        throw constexpr_logic_execution_error("storage deinit start on empty storage");
    }

    if (get_current_frame().local_values[sds.target_value] != nullptr)
    {
        throw compiler_bug("storage deinit target slot already allocated");
    }

    auto target_slot = create_local_value(sds.target_value, false);
    auto const& stored_object = storage_local->stored_object;
    target_slot->data = stored_object->data;
    target_slot->negative = stored_object->negative;
    target_slot->stage = slot_stage::full;
    target_slot->readonly = stored_object->readonly;
    target_slot->procedure = stored_object->procedure;
    target_slot->ref = stored_object->ref;
    target_slot->member_of = stored_object->member_of;
    target_slot->initializer_of = stored_object->initializer_of;
    target_slot->array_init_member_of = stored_object->array_init_member_of;
    target_slot->antestatal_static_symbol = stored_object->antestatal_static_symbol;
    target_slot->dtor = stored_object->dtor;
    target_slot->array_members = stored_object->array_members;
    target_slot->struct_members = stored_object->struct_members;
    target_slot->delegates = stored_object->delegates;
    target_slot->stored_object = stored_object;
    target_slot->storage_owner = storage_local;
    target_slot->storage_active_type = stored_object->storage_active_type;
    target_slot->storage_projection_type = object_type;
    target_slot->storage_destroy_delegate = true;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::storage_pun const& spn)
{
    auto& storage_ref_slot = get_current_frame().local_values.at(spn.from_storage);
    if (!storage_ref_slot || !storage_ref_slot->alive() || !storage_ref_slot->ref.has_value())
    {
        throw constexpr_logic_execution_error("storage_pun expects a live storage reference");
    }
    if (pointer_invalidated(storage_ref_slot->ref.value()))
    {
        throw constexpr_logic_execution_error("storage_pun on invalid storage");
    }

    auto storage_local = storage_ref_slot->ref.value().pointer_target.value().lock();
    storage_ref_slot = nullptr;
    if (!storage_local || !storage_local->alive())
    {
        throw constexpr_logic_execution_error("storage_pun on invalid storage");
    }

    if (!storage_local->storage_active_type.has_value() || storage_local->storage_active_type.value() != spn.as_type)
    {
        throw constexpr_logic_execution_error("storage_pun on storage containing a different type");
    }
    if (storage_local->stored_object == nullptr || !storage_local->stored_object->alive())
    {
        throw constexpr_logic_execution_error("storage_pun on empty storage");
    }

    auto out_ref = output_local(spn.to_reference);
    out_ref->ref = pointer_impl{.pointer_target = storage_local->stored_object};
}
quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::constexpr_allocation& quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::register_constexpr_allocation(constexpr_allocation allocation)
{
    auto const index = constexpr_allocations.size();
    constexpr_allocations.push_back(std::move(allocation));
    auto& stored = constexpr_allocations.back();
    if (stored.root)
    {
        constexpr_allocation_lookup[stored.root.get()] = index;
    }
    for (auto const& element : stored.elements)
    {
        if (element)
        {
            constexpr_allocation_lookup[element.get()] = index;
        }
    }
    return stored;
}

quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::constexpr_allocation& quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::allocation_for_pointer(pointer_impl const& ptr)
{
    local* key = nullptr;
    if (ptr.pointer_target.has_value())
    {
        auto target = ptr.pointer_target.value().lock();
        if (!target)
        {
            throw constexpr_logic_execution_error("constexpr allocator pointer refers to invalid storage");
        }
        key = target.get();
    }
    else if (ptr.one_past_the_end.has_value())
    {
        auto target = ptr.one_past_the_end.value().lock();
        if (!target)
        {
            throw constexpr_logic_execution_error("constexpr allocator pointer refers to invalid storage");
        }
        key = target.get();
    }
    else
    {
        throw constexpr_logic_execution_error("nullptr cannot be freed by constexpr allocator");
    }

    auto it = constexpr_allocation_lookup.find(key);
    if (it == constexpr_allocation_lookup.end())
    {
        throw constexpr_logic_execution_error("pointer was not produced by a constexpr allocator");
    }
    return constexpr_allocations.at(it->second);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::invalidate_local_tree(std::shared_ptr< local > const& object)
{
    if (!object)
    {
        return;
    }

    for (auto const& [_, member] : object->struct_members)
    {
        invalidate_local_tree(member);
    }
    for (auto const& member : object->array_members)
    {
        invalidate_local_tree(member);
    }
    if (object->stored_object)
    {
        auto stored = object->stored_object;
        object->stored_object = nullptr;
        invalidate_local_tree(stored);
    }

    if (object->storage_owner.has_value())
    {
        auto owner = object->storage_owner.value().lock();
        if (owner != nullptr && owner->stored_object == object)
        {
            owner->stored_object = nullptr;
            owner->storage_active_type = std::nullopt;
        }
    }

    object->stage = slot_stage::dead;
    object->storage_initiated = false;
    object->storage_active_type = std::nullopt;
    object->antestatal_static_symbol = std::nullopt;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::ensure_allocation_storage_can_be_freed(constexpr_allocation& allocation)
{
    for (auto const& storage_local : allocation.elements)
    {
        if (!storage_local || !storage_local->stored_object || !storage_local->stored_object->alive())
        {
            continue;
        }

        throw constexpr_logic_execution_error("freeing a live object during constexpr execution is not allowed");
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::do_constexpr_dealloc(type_symbol storage_type, pointer_impl ptr, std::optional< std::uint64_t > count)
{
    auto& allocation = allocation_for_pointer(ptr);

    if (allocation.freed)
    {
        throw constexpr_logic_execution_error("constexpr allocator detected a double free");
    }
    if (allocation.storage_type != storage_type)
    {
        throw constexpr_logic_execution_error("constexpr allocator deallocation type does not match the allocation type");
    }

    bool const expects_multiple = count.has_value();
    bool const allocation_is_multiple = allocation.kind == builtin_allocator_kind::constexpr_alloc_multiple;
    if (expects_multiple != allocation_is_multiple)
    {
        throw constexpr_logic_execution_error("constexpr allocator deallocation used the wrong allocator family");
    }

    if (!expects_multiple)
    {
        if (ptr.one_past_the_end.has_value())
        {
            throw constexpr_logic_execution_error("constexpr allocator single-object free requires the original base pointer");
        }
        auto const target = ptr.pointer_target.value().lock();
        if (target != allocation.root)
        {
            throw constexpr_logic_execution_error("constexpr allocator single-object free rejected an interior pointer");
        }
    }
    else
    {
        if (*count != allocation.count)
        {
            throw constexpr_logic_execution_error("constexpr allocator multi-object free used the wrong element count");
        }

        if (allocation.count == 0)
        {
            if (!ptr.one_past_the_end.has_value() || ptr.one_past_the_end.value().lock() != allocation.root)
            {
                throw constexpr_logic_execution_error("constexpr allocator zero-count free requires the original allocation pointer");
            }
        }
        else
        {
            if (ptr.one_past_the_end.has_value())
            {
                throw constexpr_logic_execution_error("constexpr allocator multi-object free requires the original base pointer");
            }
            auto const target = ptr.pointer_target.value().lock();
            if (target != allocation.elements.front())
            {
                throw constexpr_logic_execution_error("constexpr allocator multi-object free rejected an interior pointer");
            }
        }
    }

    ensure_allocation_storage_can_be_freed(allocation);
    allocation.freed = true;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_alloc const& cal)
{
    auto storage_local = create_object(cal.storage_type);
    begin_lifetime(storage_local);

    register_constexpr_allocation(constexpr_allocation{
        .kind = builtin_allocator_kind::constexpr_alloc,
        .storage_type = cal.storage_type,
        .count = 1,
        .root = storage_local,
        .elements = {storage_local},
    });

    auto result = output_local(cal.result);
    result->ref = pointer_impl{.pointer_target = storage_local};
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_alloc_multiple const& cal)
{
    auto const count = consume_u64(cal.count);
    auto root = create_object(cal.storage_type);
    root->array_members.clear();
    root->array_members.reserve(static_cast< std::size_t >(count));

    std::vector< std::shared_ptr< local > > elements;
    elements.reserve(static_cast< std::size_t >(count));
    for (std::uint64_t i = 0; i < count; ++i)
    {
        auto element = create_object(cal.storage_type);
        begin_lifetime(element);
        element->member_of = root;
        root->array_members.push_back(element);
        elements.push_back(std::move(element));
    }
    begin_lifetime(root);

    register_constexpr_allocation(constexpr_allocation{
        .kind = builtin_allocator_kind::constexpr_alloc_multiple,
        .storage_type = cal.storage_type,
        .count = count,
        .root = root,
        .elements = elements,
    });

    auto result = output_local(cal.result);
    if (count == 0)
    {
        result->ref = pointer_impl{.one_past_the_end = root};
    }
    else
    {
        result->ref = pointer_impl{.pointer_target = root->array_members.front()};
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_dealloc const& cal)
{
    auto ptr = load_as_pointer(cal.pointer, true);
    do_constexpr_dealloc(cal.storage_type, ptr, std::nullopt);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_dealloc_multiple const& cal)
{
    auto ptr = load_as_pointer(cal.pointer, true);
    auto const count = consume_u64(cal.count);
    do_constexpr_dealloc(cal.storage_type, ptr, count);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::get_global_storage const& ggs)
{
    do_get_global_storage(ggs.symbol, ggs.target_ref);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::get_antestatal_ref const& gar)
{
    do_get_antestatal_ref(gar.symbol, gar.target_ref);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::do_get_global_storage(type_symbol symbol, local_index target_ref)
{
    auto const target_type = get_local_type(target_ref);
    if (!is_ref(target_type))
    {
        throw compiler_bug("GET_GLOBAL_STORAGE requires a reference-typed destination");
    }

    auto const storage_type = remove_ref(target_type);
    if (!typeis< storage >(storage_type))
    {
        throw compiler_bug("GET_GLOBAL_STORAGE requires a destination of type QUAL& STORAGE(T)");
    }

    auto storage_local = get_or_create_global_storage(symbol, storage_type);
    auto out_ref = output_local(target_ref);
    out_ref->ref = pointer_impl{.pointer_target = storage_local};
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::do_get_antestatal_ref(type_symbol symbol, local_index target_ref)
{
    auto const target_type = get_local_type(target_ref);
    if (!is_ref(target_type))
    {
        throw compiler_bug("GET_ANTESTATAL_REF requires a reference-typed destination");
    }

    auto const object_type = remove_ref(target_type);
    auto object = get_or_create_antestatal_global(symbol, object_type);
    auto out_ref = output_local(target_ref);
    out_ref->ref = pointer_impl{.pointer_target = object};
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::make_reference const& mrf)
{
    auto ptr = get_pointer_to(current_frame_index(), mrf.value_index);

    auto ref_type = get_local_type(mrf.reference_index);
    if (!is_ref(ref_type))
    {
        throw compiler_bug("Attempt to make reference by non-reference type");
    }

    if (get_current_frame().local_values[mrf.reference_index])
    {
        throw compiler_bug("Attempt to overwrite reference (A)");
    }

    auto local_ptr = output_local(mrf.reference_index);
    local_ptr->ref = ptr;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::jump const& jmp)
{
    transition(jmp.target);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::branch const& brn)
{
    auto reg = brn.condition;

    auto data = consume_local_as_data(reg);

    if (data == std::vector< std::byte >{std::byte{0}})
    {
        transition(brn.target_false);
    }
    else
    {
        transition(brn.target_true);
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::runtime_constexpr const& rce)
{
    transition(rce.target_constexpr);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::initguard_try_acquire const& ita)
{
    do_initguard_try_acquire(ita.symbol, ita.target_lock, ita.target_acquired, ita.target_already_initialized);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::do_initguard_try_acquire(type_symbol symbol, local_index target_lock, block_index target_acquired, block_index target_already_initialized)
{
    if (has_constexpr_antestatal_global(symbol))
    {
        transition(target_already_initialized);
        return;
    }

    auto guard = get_or_create_initguard(symbol);

    switch (get_initguard_state(guard))
    {
    case initguard_state::initialized:
        transition(target_already_initialized);
        return;
    case initguard_state::uninitialized: {
        auto lock = output_local(target_lock);
        set_initguard_lock(lock, guard);
        set_initguard_state(guard, initguard_state::initializing);
        transition(target_acquired);
        return;
    }
    case initguard_state::initializing:
        throw constexpr_logic_execution_error("INITGUARD_TRY_ACQUIRE recursion detected for " + quxlang::to_string(symbol));
    }

    throw compiler_bug("unknown initguard state");
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::cast_ptrref const& cst)
{
    auto& local_ptr_base = get_current_frame().local_values.at(cst.source_index);
    auto local_ptr_result = output_local(cst.target_index);

    if (!local_ptr_base)
    {
        throw constexpr_logic_execution_error("Error executing <cast_ptrref>: accessing deallocated storage");
    }

    if (!local_ptr_base->alive())
    {
        throw constexpr_logic_execution_error("Error executing <cast_ptrref>: accessing dealived storage");
    }

    local_ptr_result->ref = local_ptr_base->ref;

    end_lifetime(local_ptr_base);
    local_ptr_base = nullptr;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_set_result const& csr)
{
    this->constexpr_result_v = consume_local_as_data(csr.target);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_set_result2 const& csr)
{
    auto& local_ptr = get_current_frame().local_values.at(csr.target);
    auto target_type = get_local_type(csr.target);

    std::shared_ptr< local > materialize_from = local_ptr;
    type_symbol materialize_type = target_type;
    if (csr.target_mode == vmir2::constexpr_result_target_mode::referenced_object)
    {
        if (!is_ref(target_type))
        {
            throw constexpr_logic_execution_error("CE_SETRESULT_ANTESTATAL referenced_object requires a reference target");
        }
        if (local_ptr == nullptr || !local_ptr->alive() || !local_ptr->ref.has_value() || !local_ptr->ref->pointer_target.has_value())
        {
            throw constexpr_logic_execution_error("CE_SETRESULT_ANTESTATAL referenced_object requires a live object reference");
        }
        materialize_from = local_ptr->ref->pointer_target->lock();
        if (materialize_from == nullptr || !materialize_from->alive())
        {
            throw constexpr_logic_execution_error("CE_SETRESULT_ANTESTATAL referenced_object points at an invalid object");
        }
        materialize_type = remove_ref(target_type);
    }

    constexpr_result_root = materialize_from;
    auto value = materialize_antestatal_value(materialize_from, materialize_type);
    this->constexpr_result_antestatal_values[csr.result_id] = value;
    if (csr.result_id == 0)
    {
        this->constexpr_result_antestatal = std::move(value);
    }
    consume_local(csr.target);
    constexpr_result_root.reset();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_make_proxy const& cmp)
{
    auto proxy = output_local(cmp.target);
    proxy->constexpr_proxy_output_id = cmp.result_id;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_output_byte const& cob)
{
    auto proxy_slot = get_current_frame().local_values[cob.proxy];
    if (proxy_slot == nullptr || !proxy_slot->alive())
    {
        throw constexpr_logic_execution_error("CE_OUTPUT_BYTE requires a live constexpr proxy reference slot");
    }
    auto value = use_data(cob.value);
    if (value.size() != 1)
    {
        throw constexpr_logic_execution_error("CE_OUTPUT_BYTE requires a single-byte value");
    }

    auto proxy_type = get_local_type(cob.proxy);
    if (!is_ref(proxy_type))
    {
        throw constexpr_logic_execution_error("CE_OUTPUT_BYTE requires a constexpr proxy reference");
    }
    if (!proxy_slot->ref.has_value() || !proxy_slot->ref->pointer_target.has_value())
    {
        throw constexpr_logic_execution_error("CE_OUTPUT_BYTE requires a valid constexpr proxy reference");
    }
    auto proxy_object = proxy_slot->ref->pointer_target->lock();
    if (proxy_object == nullptr || !proxy_object->alive() || !proxy_object->constexpr_proxy_output_id.has_value())
    {
        throw constexpr_logic_execution_error("CE_OUTPUT_BYTE requires a live constexpr proxy");
    }

    consume_local(cob.proxy);
    consume_local(cob.value);
    constexpr_result_serialoid_values[*proxy_object->constexpr_proxy_output_id].bytes.push_back(value.front());
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::load_const_value const& lcv)
{
    auto& frame = get_current_frame();
    auto target_type = get_local_type(lcv.target);
    auto output_obj = output(lcv.target);
    if (!output_obj->struct_members.contains("__start") || !output_obj->struct_members.contains("__end"))
    {
        auto storage_owner = output_obj->storage_owner;
        auto storage_projection_type = output_obj->storage_projection_type;
        auto storage_destroy_delegate = output_obj->storage_destroy_delegate;
        init_storage(output_obj, target_type);
        output_obj->storage_owner = std::move(storage_owner);
        output_obj->storage_projection_type = std::move(storage_projection_type);
        output_obj->storage_destroy_delegate = storage_destroy_delegate;
    }
    auto const_data_bytes = lcv.value;
    if (const_data_bytes.empty())
    {
        const_data_bytes.push_back(std::byte{0});
    }
    auto target_data = constdata(const_data_bytes);
    auto start_ptr_impl = make_pointer_to(target_data->array_members.at(0));
    auto end_ptr_impl = lcv.value.empty() ? start_ptr_impl : pointer_arith(start_ptr_impl, lcv.value.size(), void_type{});
    auto start_ptr_object = output_obj->struct_members["__start"];
    auto end_ptr_object = output_obj->struct_members["__end"];
    begin_lifetime(start_ptr_object);
    start_ptr_object->ref = start_ptr_impl;
    begin_lifetime(end_ptr_object);
    end_ptr_object->ref = end_ptr_impl;
    begin_lifetime(output_obj);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::canonicalize_float const& cpf)
{
    auto source_type = get_local_type(cpf.source);
    auto result_type = get_local_type(cpf.result);
    if (source_type != result_type || !source_type.type_is< float_type >())
    {
        throw constexpr_logic_execution_error("FCANON requires matching floating point source and result types");
    }

    auto source_data = consume_local_as_data(cpf.source);
    auto result = output_local(cpf.result);
    local_set_data(result, bytemath::fixed_float_canonicalize_nan_le(get_fixed_float_options(source_type), std::move(source_data)));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::get_value_byte const& gvb)
{
    auto reference_type = get_local_type(gvb.source_reference);
    if (!is_ref(reference_type))
    {
        throw constexpr_logic_execution_error("GET_BYTE requires a reference source");
    }

    auto source_ref = consume_local(gvb.source_reference);
    if (!source_ref->ref.has_value() || pointer_invalidated(*source_ref->ref))
    {
        throw constexpr_logic_execution_error("GET_BYTE requires a valid reference source");
    }

    auto source = source_ref->ref->pointer_target.value().lock();
    if (!source || !source->alive())
    {
        throw constexpr_logic_execution_error("GET_BYTE source is not alive");
    }
    if (gvb.offset >= source->data.size())
    {
        throw constexpr_logic_execution_error("GET_BYTE offset is out of bounds");
    }

    auto result = output_local(gvb.result);
    local_set_data(result, {source->data.at(static_cast< std::size_t >(gvb.offset))});
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::set_value_byte const& svb)
{
    auto reference_type = get_local_type(svb.target_reference);
    auto value_type = get_local_type(svb.value);
    if (!is_ref(reference_type) || !value_type.type_is< byte_type >())
    {
        throw constexpr_logic_execution_error("SET_BYTE requires a reference target and BYTE value");
    }

    auto target_ref = consume_local(svb.target_reference);
    auto value = consume_local_as_data(svb.value);
    if (value.size() != 1)
    {
        throw constexpr_logic_execution_error("SET_BYTE value must be exactly one byte");
    }
    if (!target_ref->ref.has_value() || pointer_invalidated(*target_ref->ref))
    {
        throw constexpr_logic_execution_error("SET_BYTE requires a valid reference target");
    }

    auto target = target_ref->ref->pointer_target.value().lock();
    if (!target || !target->alive())
    {
        throw constexpr_logic_execution_error("SET_BYTE target is not alive");
    }
    if (target->readonly)
    {
        throw constexpr_logic_execution_error("SET_BYTE target is read-only");
    }
    if (svb.offset >= target->data.size())
    {
        throw constexpr_logic_execution_error("SET_BYTE offset is out of bounds");
    }

    target->data.at(static_cast< std::size_t >(svb.offset)) = value.front();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::make_pointer_to const& mpt)
{
    auto& frame = get_current_frame();
    auto& source = frame.local_values.at(mpt.of_index);
    if (!source || !source->alive())
    {
        throw constexpr_logic_execution_error("Expected source value to be valid");
    }
    pointer_impl ptr = get_pointer_to(stack.size() - 1, mpt.of_index);
    if (ptr.pointer_target.value().expired())
    {
        throw std::logic_error("creating a pointer to non value???");
    }
    auto ptrval = output_local(mpt.pointer_index);
    ptrval->ref = ptr;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::dereference_pointer const& drp)
{
    auto& frame = get_current_frame();

    auto ptr = consume_local(drp.from_pointer);

    auto ref = output_local(drp.to_reference);

    if (!ptr->ref.has_value())
    {
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::cout << "ptr object id: " << ptr->object_id << std::endl;
        }
        throw std::logic_error("pointer missing value?");
    }
    if (pointer_invalidated(ptr->ref.value()))
    {
        throw constexpr_logic_execution_error("dereference pointer to invalid storage location");
    }

    auto pointer_target = ptr->ref.value().pointer_target.value().lock();

    if (!pointer_target)
    {
        throw constexpr_logic_execution_error("dereference pointer to invalid storage location");
    }

    pointer_impl pointer_to_target = pointer_impl{.pointer_target = pointer_target};

    ref->ref = pointer_to_target;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::load_from_ref const& lfr)
{
    auto& slot = get_current_frame().local_values.at(lfr.from_reference);

    if (!slot || !slot->alive())
    {
        throw constexpr_logic_execution_error("Error executing <load_from_ref>: accessing deallocated storage");
    }
    assert(slot->ref.has_value());
    if (pointer_invalidated(*slot->ref))
    {
        throw constexpr_logic_execution_error("Error executing <load_from_ref>: accessing deallocated storage");
    }

    auto load_from_ptr = slot->ref->pointer_target.value().lock();
    slot = nullptr;
    if (!load_from_ptr)
    {
        throw constexpr_logic_execution_error("Error executing <load_from_ref>: accessing deallocated storage");
    }
    auto& load_from = *load_from_ptr;
    if (!load_from.alive() && access_to_antestatal_local(load_from_ptr).has_value())
    {
        throw constexpr_logic_execution_error("cannot read symbolic antestatal static during constexpr evaluation");
    }

    auto target_slot = output_local(lfr.to_value);
    target_slot->data = load_from.data;
    target_slot->ref = load_from.ref;
    target_slot->constexpr_proxy_output_id = load_from.constexpr_proxy_output_id;
    target_slot->interface_value = load_from.interface_value;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::ret const& ret)
{
    // transition_normal_exit will push a destructor call onto the stack and then return false,
    // or if no more destructors are needed, it will return true.
    // When it returns true, we are done running destructors and can pop the current stack frame.

    std::string back_addr = quxlang::to_string(stack.back().address.func.get());
    auto exit_ok = transition_normal_exit();
    if (exit_ok)
    {
        stack.pop_back();
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_sub const& sub)
{
    exec_fixed_int_binary_op(fixed_int_instruction::sub, sub.a, sub.b, sub.result, &bytemath::fixed_int_sub_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_add const& add)
{
    exec_fixed_int_binary_op(fixed_int_instruction::add, add.a, add.b, add.result, &bytemath::fixed_int_add_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::iconv const& icv)
{
    auto& frame = get_current_frame();

    std::shared_ptr< local > from_local = consume_local(icv.from);
    std::shared_ptr< local > to_local = output_local(icv.to);

    // Retrieve data references
    auto& from_data = from_local->data;
    auto& to_data = to_local->data;

    type_symbol from_type = get_local_type(icv.from);
    type_symbol to_type = get_local_type(icv.to);

    bytemath::fixed_int_options from_opt{};
    bytemath::fixed_int_options to_opt{};

    if (to_type.type_is< byte_type >())
    {
        to_opt.bits = 8;
        to_opt.has_sign = false;
    }
    else
    {
        assert(to_type.type_is< int_type >());
        int_type const& int_type_info = to_type.get_as< int_type >();
        to_opt.bits = int_type_info.bits;
        to_opt.has_sign = int_type_info.has_sign;
    }

    if (from_type.type_is< byte_type >())
    {
        from_opt.bits = 8;
        from_opt.has_sign = false;
    }
    else
    {
        assert(from_type.type_is< int_type >());
        int_type const& int_type_info = from_type.get_as< int_type >();
        from_opt.bits = int_type_info.bits;
        from_opt.has_sign = int_type_info.has_sign;
    }

    if (icv.convtype == conversion_class::partial)
    {
        to_opt.overflow_undefined = false;
    }
    else
    {
        // conversion class checked triggers a fault, but the result is the same for constexpr
        to_opt.overflow_undefined = true;
    }

    bytemath::int_result res = bytemath::fixed_int_convert(from_opt, to_opt, from_data);
    // Perform two's complement addition in little-endian order

    if (res.result_is_undefined)
    {
        if (icv.convtype == conversion_class::checked)
        {
            raise_fault("CHECKED_CONVERSION_FAULT");
            return;
        }
        else
        {
            throw constexpr_logic_execution_error("error executing IADD: undefined behavior");
        }
    }

    to_data = res.data_bytes;

    return;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_mul const& mul)
{
    exec_fixed_int_binary_op(fixed_int_instruction::mul, mul.a, mul.b, mul.result, &bytemath::fixed_int_mul_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_div const& div)
{
    exec_fixed_int_binary_op(fixed_int_instruction::div, div.a, div.b, div.result, &bytemath::fixed_int_div_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_mod const& mod)
{
    exec_fixed_int_binary_op(fixed_int_instruction::mod, mod.a, mod.b, mod.result, &bytemath::fixed_int_mod_le);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_int_add const& op)
{
    exec_mut_fixed_int_binary_op(fixed_int_instruction::add, op.target, op.value, &bytemath::fixed_int_add_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_int_sub const& op)
{
    exec_mut_fixed_int_binary_op(fixed_int_instruction::sub, op.target, op.value, &bytemath::fixed_int_sub_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_int_mul const& op)
{
    exec_mut_fixed_int_binary_op(fixed_int_instruction::mul, op.target, op.value, &bytemath::fixed_int_mul_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_int_div const& op)
{
    exec_mut_fixed_int_binary_op(fixed_int_instruction::div, op.target, op.value, &bytemath::fixed_int_div_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_int_mod const& op)
{
    exec_mut_fixed_int_binary_op(fixed_int_instruction::mod, op.target, op.value, &bytemath::fixed_int_mod_le);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_add const& op)
{
    exec_fixed_float_binary_op(fixed_float_instruction::add, op.a, op.b, op.result, &bytemath::fixed_float_add_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_sub const& op)
{
    exec_fixed_float_binary_op(fixed_float_instruction::sub, op.a, op.b, op.result, &bytemath::fixed_float_sub_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_mul const& op)
{
    exec_fixed_float_binary_op(fixed_float_instruction::mul, op.a, op.b, op.result, &bytemath::fixed_float_mul_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_div const& op)
{
    exec_fixed_float_binary_op(fixed_float_instruction::div, op.a, op.b, op.result, &bytemath::fixed_float_div_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_float_add const& op)
{
    exec_mut_fixed_float_binary_op(fixed_float_instruction::add, op.target, op.value, &bytemath::fixed_float_add_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_float_sub const& op)
{
    exec_mut_fixed_float_binary_op(fixed_float_instruction::sub, op.target, op.value, &bytemath::fixed_float_sub_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_float_mul const& op)
{
    exec_mut_fixed_float_binary_op(fixed_float_instruction::mul, op.target, op.value, &bytemath::fixed_float_mul_le);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_float_div const& op)
{
    exec_mut_fixed_float_binary_op(fixed_float_instruction::div, op.target, op.value, &bytemath::fixed_float_div_le);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_from_int const& op)
{
    require_valid_input_precondition(op.source);
    require_valid_output_precondition(op.result);

    auto source_type = get_local_type(op.source);
    auto result_type = get_local_type(op.result);
    if (!result_type.type_is< float_type >() || (!source_type.type_is< int_type >() && !source_type.type_is< byte_type >()))
    {
        throw std::runtime_error("ITOF requires an integer source and floating point result");
    }

    auto source_data = consume_local_as_data(op.source);
    auto result_local = output_local(op.result);
    auto result = bytemath::fixed_float_from_int_le(get_fixed_float_options(result_type), get_fixed_int_options(source_type), std::move(source_data), op.require_exact);
    if (result.result_is_undefined)
    {
        throw constexpr_logic_execution_error("error executing ITOF: undefined behavior");
    }
    if (op.require_exact && !result.result_is_exact)
    {
        throw constexpr_logic_execution_error("error executing ITOF: integer cannot be exactly represented by floating point type; use APPROXIMATE");
    }
    if (!op.require_exact && bytemath::unpack_fixed_float(get_fixed_float_options(result_type), result.data_bytes).kind == bytemath::fixed_float_kind::infinity)
    {
        throw constexpr_logic_execution_error("error executing ITOF: integer is outside the finite floating point range");
    }

    local_set_data(result_local, std::move(result.data_bytes));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::store_to_ref const& str)
{
    auto from_type = get_local_type(str.from_value);
    auto to_type = get_local_type(str.to_reference);

    auto from_local = consume_local(str.from_value);
    auto to_local = consume_local(str.to_reference);

    pointer_impl from_ptr;
    pointer_impl to_ptr;

    if (is_ref(from_type))
    {
        if (!from_local->ref.has_value())
        {
            throw compiler_bug("Reference local missing ref value");
        }
        from_ptr = *from_local->ref;
    }
    else
    {
        from_ptr.pointer_target = from_local;
    }

    if (!is_ref(to_type))
    {
        throw compiler_bug("store_to_ref target must be reference type");
    }

    if (!to_local->ref.has_value())
    {
        throw compiler_bug("Reference local missing ref value");
    }
    to_ptr = *to_local->ref;

    if (from_ptr.pointer_target.value().expired())
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: loading from deallocated storage");
    }
    if (to_ptr.pointer_target.value().expired())
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: storing into deallocated storage");
    }
    if (is_ref(from_type) && pointer_invalidated(from_ptr))
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: loading from invalidated storage");
    }
    if (pointer_invalidated(to_ptr))
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: storing into invalidated storage");
    }

    auto& from_ptr_target = *from_ptr.pointer_target.value().lock();
    auto& to_ptr_target = *to_ptr.pointer_target.value().lock();

    if (to_ptr_target.readonly)
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: storing into read-only object");
    }

    std::size_t write_size = from_ptr_target.data.size();

    for (auto i = 0; i < write_size; i++)
    {
        auto to_ptr_offset = i;
        if (to_ptr_offset >= to_ptr_target.data.size())
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                std::cout << "str: from: " << from_ptr_target.object_id << " to: " << to_ptr_target.object_id << std::endl;
            }
            throw constexpr_logic_execution_error("Error executing <store_to_ref>: out of bounds write");
        }
        to_ptr_target.data[to_ptr_offset] = from_ptr_target.data[i];
    }

    to_ptr_target.ref = from_ptr_target.ref;
    to_ptr_target.constexpr_proxy_output_id = from_ptr_target.constexpr_proxy_output_id;
    to_ptr_target.interface_value = from_ptr_target.interface_value;

    return;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::load_const_int const& lci)
{
    auto intval = output_local(lci.target);

    type_symbol int_type_v;

    int_type_v = get_current_frame().ir3->local_types.at(lci.target).type;

    if (int_type_v.type_is< nvalue_slot >())
    {
        auto copy = int_type_v.get_as< nvalue_slot >().target;

        int_type_v = copy;
    }

    if (!int_type_v.type_is< int_type >() && !int_type_v.type_is< byte_type >())
    {
        throw compiler_bug("Expected INTEGER or BYTE type for load_const_int");
    }

    auto& data = intval->data;
    data.resize(get_type_size(int_type_v));

    std::string str = lci.value;

    // Determine sign (if signed)
    bool negative = false;

    if (!str.empty() && str[0] == '-')
    {
        negative = true;
        str.erase(str.begin());
    }

    // Remove leading zeros
    while (str.size() > 1 && str[0] == '0')
    {
        str.erase(str.begin());
    }

    // If empty after removing zeros, it's zero
    if (str.empty())
    {
        std::fill(data.begin(), data.end(), std::byte(0));
        return;
    }

    // Convert to little-endian binary by dividing by 256 until str = "0"
    std::vector< std::byte > result;
    {
        std::string temp = str;
        while (!(temp.size() == 1 && temp[0] == '0'))
        {
            std::uint32_t carry = 0;
            for (size_t i = 0; i < temp.size(); i++)
            {
                std::uint32_t x = carry * 10 + (temp[i] - '0');
                std::uint32_t q = x / 256; // quotient
                std::uint32_t r = x % 256; // remainder
                temp[i] = static_cast< char >('0' + q);
                carry = r;
            }
            // Remove leading zeros
            while (temp.size() > 1 && temp[0] == '0')
            {
                temp.erase(temp.begin());
            }
            result.push_back(static_cast< std::byte >(static_cast< uint8_t >(carry)));
        }
    }

    // Check if the integer fits into the allocated size
    if (result.size() > data.size())
    {
        // The integer does not fit into the target type size
        throw std::overflow_error("Integer too large for the target type");
    }

    // Copy the bytes
    std::copy(result.begin(), result.end(), data.begin());
    if (result.size() < data.size())
    {
        // Zero-pad the rest
        std::fill(data.begin() + result.size(), data.end(), std::byte(0));
    }

    // Handle negatives if signed
    if (negative)
    {
        // Convert to two's complement
        for (auto& byte : data)
        {
            byte = ~byte;
        }
        std::uint16_t carry = 1;
        for (std::size_t i = 0; i < data.size() && carry; i++)
        {
            std::uint16_t val = static_cast< std::uint16_t >(static_cast< std::uint8_t >(data[i])) + carry;
            data[i] = static_cast< std::byte >(static_cast< uint8_t >(val & 0xFF));
            carry = (val > 0xFF) ? 1 : 0;
        }
    }

    return;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::load_const_float const& lcf)
{
    auto floatval = output_local(lcf.target);

    type_symbol float_type_v = get_current_frame().ir3->local_types.at(lcf.target).type;
    if (float_type_v.type_is< nvalue_slot >())
    {
        float_type_v = float_type_v.get_as< nvalue_slot >().target;
    }
    if (!float_type_v.type_is< float_type >())
    {
        throw compiler_bug("Expected FLOAT type for load_const_float");
    }

    auto opts = get_fixed_float_options(float_type_v);
    auto result = bytemath::fixed_float_from_decimal_string(opts, lcf.value, lcf.require_exact);
    if (result.result_is_undefined)
    {
        throw constexpr_logic_execution_error("error executing INIT_FLOAT: undefined behavior");
    }
    if (lcf.require_exact && !result.result_is_exact)
    {
        throw constexpr_logic_execution_error("error executing INIT_FLOAT: numeric literal cannot be exactly represented by floating point type; use APPROXIMATE");
    }
    if (result.data_bytes.size() != get_type_size(float_type_v))
    {
        throw compiler_bug("INIT_FLOAT: bytemath returned unexpected result size");
    }
    floatval->data = std::move(result.data_bytes);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::cmp_eq const& ceq)
{
    auto a_type = get_local_type(ceq.a);
    if (a_type.type_is< float_type >())
    {
        auto b_type = get_local_type(ceq.b);
        if (a_type != b_type)
        {
            throw std::runtime_error("FCMP_EQ: type mismatch among operands");
        }
        auto a = consume_local_as_data(ceq.a);
        auto b = consume_local_as_data(ceq.b);
        auto result = bytemath::fixed_float_qux_eq_le(get_fixed_float_options(a_type), std::move(a), std::move(b));
        set_data(ceq.result, {std::byte(result.result ? 1 : 0)});
        return;
    }

    auto a = consume_local_as_data(ceq.a);
    auto b = consume_local_as_data(ceq.b);

    assert(a.size() == b.size());

    if (a == b)
    {
        set_data(ceq.result, {std::byte(1)});
    }
    else
    {
        set_data(ceq.result, {std::byte(0)});
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::cmp_ne const& cne)
{
    auto a_type = get_local_type(cne.a);
    if (a_type.type_is< float_type >())
    {
        auto b_type = get_local_type(cne.b);
        if (a_type != b_type)
        {
            throw std::runtime_error("FCMP_NE: type mismatch among operands");
        }
        auto a = consume_local_as_data(cne.a);
        auto b = consume_local_as_data(cne.b);
        auto result = bytemath::fixed_float_qux_eq_le(get_fixed_float_options(a_type), std::move(a), std::move(b));
        set_data(cne.result, {std::byte(result.result ? 0 : 1)});
        return;
    }

    auto a = consume_local_as_data(cne.a);
    auto b = consume_local_as_data(cne.b);

    assert(a.size() == b.size());

    if (a != b)
    {
        set_data(cne.result, {std::byte(1)});
    }
    else
    {
        set_data(cne.result, {std::byte(0)});
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::cmp_lt const& clt)
{
    auto a_type = get_local_type(clt.a);
    if (a_type.type_is< float_type >())
    {
        auto b_type = get_local_type(clt.b);
        if (a_type != b_type)
        {
            throw std::runtime_error("FCMP_LT: type mismatch among operands");
        }
        auto a = consume_local_as_data(clt.a);
        auto b = consume_local_as_data(clt.b);
        auto result = bytemath::fixed_float_qux_lt_le(get_fixed_float_options(a_type), std::move(a), std::move(b));
        set_data(clt.result, {std::byte(result.result ? 1 : 0)});
        return;
    }

    auto a = consume_local_as_data(clt.a);
    auto b = consume_local_as_data(clt.b);

    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        std::cout << "CLT " << bytemath::detail::le_to_string_raw(a) << " " << bytemath::detail::le_to_string_raw(b) << std::endl;
    }

    for (std::size_t i = a.size() - 1; true; i--)
    {
        if (a[i] < b[i])
        {
            set_data(clt.result, {std::byte(1)});
            // std::cout << "CLT: " << bytemath::detail::le_to_string_raw(a) << " < " << bytemath::detail::le_to_string_raw(b) << std::endl;
            return;
        }
        if (a[i] > b[i])
        {
            set_data(clt.result, {std::byte(0)});
            // std::cout << "CLT: " << bytemath::detail::le_to_string_raw(a) << " > " << bytemath::detail::le_to_string_raw(b) << std::endl;
            return;
        }
        if (i == 0)
        {
            break;
        }
    }

    set_data(clt.result, {std::byte(0)});
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::cmp_ge const& cge)
{
    auto a_type = get_local_type(cge.a);
    if (a_type.type_is< float_type >())
    {
        auto b_type = get_local_type(cge.b);
        if (a_type != b_type)
        {
            throw std::runtime_error("FCMP_GE: type mismatch among operands");
        }
        auto a = consume_local_as_data(cge.a);
        auto b = consume_local_as_data(cge.b);
        auto result = bytemath::fixed_float_qux_ge_le(get_fixed_float_options(a_type), std::move(a), std::move(b));
        set_data(cge.result, {std::byte(result.result ? 1 : 0)});
        return;
    }

    auto a = consume_local_as_data(cge.a);
    auto b = consume_local_as_data(cge.b);

    assert(a.size() == b.size());

    // Compare from most-significant byte to least
    for (std::size_t i = a.size() - 1; true; i--)
    {
        if (a[i] > b[i])
        {
            set_data(cge.result, {std::byte(1)});
            return;
        }
        if (a[i] < b[i])
        {
            set_data(cge.result, {std::byte(0)});
            return;
        }
        if (i == 0)
        {
            break;
        }
    }

    set_data(cge.result, {std::byte(1)});
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_ieee_eq const& op)
{
    exec_fixed_float_compare_op("IEEE_FEQ", op.a, op.b, op.result, &bytemath::fixed_float_ieee_eq_le);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_ieee_ne const& op)
{
    exec_fixed_float_compare_op("IEEE_FNE", op.a, op.b, op.result, &bytemath::fixed_float_ieee_ne_le);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_ieee_lt const& op)
{
    exec_fixed_float_compare_op("IEEE_FLT", op.a, op.b, op.result, &bytemath::fixed_float_ieee_lt_le);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::float_ieee_gt const& op)
{
    exec_fixed_float_compare_op("IEEE_FGT", op.a, op.b, op.result, &bytemath::fixed_float_ieee_gt_le);
}

// ===== Bitwise operations handlers =====

static std::size_t get_bit_width_for_type(const quxlang::type_symbol& ty)
{
    if (typeis< quxlang::int_type >(ty))
    {
        return ty.get_as< quxlang::int_type >().bits;
    }
    if (typeis< quxlang::byte_type >(ty))
    {
        return 8;
    }
    // Fallback: use full byte width of storage
    return 8 * ((typeis< quxlang::ptrref_type >(ty)) ? 0 : 0);
}

static std::size_t bytes_for_bits(std::size_t bits)
{
    return (bits + 7) / 8;
}

static void bitwise_byte_op(std::vector< std::byte >& out, std::vector< std::byte > const& a, std::vector< std::byte > const& b, auto fn)
{
    out.resize(std::max(a.size(), b.size()));
    for (std::size_t i = 0; i < out.size(); ++i)
    {
        std::uint8_t av = (i < a.size()) ? static_cast< std::uint8_t >(a[i]) : 0;
        std::uint8_t bv = (i < b.size()) ? static_cast< std::uint8_t >(b[i]) : 0;
        out[i] = static_cast< std::byte >(fn(av, bv) & 0xFF);
    }
}

static void mut_bitwise_byte_op(std::vector< std::byte >& target, std::vector< std::byte > const& b, auto fn)
{
    if (b.size() > target.size())
    {
        target.resize(b.size(), std::byte{0});
    }
    for (std::size_t i = 0; i < target.size(); ++i)
    {
        std::uint8_t av = static_cast< std::uint8_t >(target[i]);
        std::uint8_t bv = (i < b.size()) ? static_cast< std::uint8_t >(b[i]) : 0;
        target[i] = static_cast< std::byte >(fn(av, bv) & 0xFF);
    }
}

static void bitwise_not_inplace(std::vector< std::byte >& v)
{
    for (auto& by : v)
    {
        by = static_cast< std::byte >(~static_cast< std::uint8_t >(by));
    }
}

static std::vector< std::byte > truncate_to_bits(std::vector< std::byte > data, std::size_t bits)
{
    if (bits == 0)
    {
        return {};
    }
    auto truncated = quxlang::bytemath::detail::le_truncate_raw(std::move(data), bits);
    // Ensure vector has exact byte size for given bits
    truncated.resize((bits + 7) / 8, std::byte{0});
    if (bits % 8 != 0)
    {
        std::uint8_t mask = (1u << (bits % 8)) - 1u;
        std::uint8_t top = static_cast< std::uint8_t >(truncated.back());
        truncated.back() = static_cast< std::byte >(top & mask);
    }
    return truncated;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_and const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >(av & bv);
                    });
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_or const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >(av | bv);
                    });
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_xor const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >(av ^ bv);
                    });
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_nand const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >(av & bv);
                    });
    bitwise_not_inplace(out);
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_nor const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >(av | bv);
                    });
    bitwise_not_inplace(out);
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_nxor const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >(av ^ bv);
                    });
    bitwise_not_inplace(out);
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_implies const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >((~av) | bv);
                    });
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_implied const& op)
{
    auto a = consume_local_as_data(op.a);
    auto b = consume_local_as_data(op.b);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    std::vector< std::byte > out;
    bitwise_byte_op(out, a, b,
                    [](std::uint8_t av, std::uint8_t bv)
                    {
                        return static_cast< std::uint8_t >(av | (~bv));
                    });
    out = truncate_to_bits(std::move(out), bits);
    set_data(op.result, std::move(out));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_mut_bitwise_binary_op(local_index target_slot, local_index value_slot, mut_bitwise_binary_op op, bool invert)
{
    auto target = load_from_reference(target_slot, true);
    auto value = consume_local_as_data(value_slot);
    auto target_type = remove_ref(get_local_type(target_slot));
    std::size_t bits = get_bit_width_for_type(target_type);

    mut_bitwise_byte_op(target->data, value, op);
    if (invert)
    {
        bitwise_not_inplace(target->data);
    }
    local_set_data(target, truncate_to_bits(std::move(target->data), bits));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_and const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return av & bv; }, false);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_or const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return av | bv; }, false);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_xor const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return av ^ bv; }, false);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_nand const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return av & bv; }, true);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_nor const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return av | bv; }, true);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_nxor const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return av ^ bv; }, true);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_implies const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return static_cast< std::uint8_t >((~av) | bv); }, false);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_implied const& op)
{
    exec_mut_bitwise_binary_op(op.target, op.value, [](std::uint8_t av, std::uint8_t bv) -> std::uint8_t { return static_cast< std::uint8_t >(av | (~bv)); }, false);
}

static std::uint64_t bytes_to_u64(const std::vector< std::byte >& data)
{
    auto [v, ok] = quxlang::bytemath::le_to_u< std::uint64_t >(data);
    if (!ok)
    {
        throw std::overflow_error("shift amount too large");
    }
    return v;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_shift_up const& op)
{
    auto value = consume_local_as_data(op.value);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto result_type = get_local_type(op.result);

    std::size_t bits = get_bit_width_for_type(result_type);
    if (bits == 0)
    {
        set_data(op.result, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes);
    bytemath::fixed_int_options opts{};
    opts.bits = bits;
    opts.overflow_undefined = true;

    auto shifted = bytemath::fixed_int_shift_up_le(opts, std::move(value), amt);
    if (shifted.result_is_undefined)
    {
        throw constexpr_logic_execution_error("undefined behavior, shift amount overflow");
    }
    set_data(op.result, std::move(shifted.data_bytes));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_shift_down const& op)
{
    auto value = consume_local_as_data(op.value);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto result_type = get_local_type(op.result);

    std::size_t bits = get_bit_width_for_type(result_type);
    if (bits == 0)
    {
        set_data(op.result, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes);
    bytemath::fixed_int_options opts{};
    opts.bits = bits;
    opts.overflow_undefined = true;

    auto shifted = bytemath::fixed_int_shift_down_le(opts, std::move(value), amt);
    if (shifted.result_is_undefined)
    {
        throw constexpr_logic_execution_error("undefined behavior, shift amount overflow");
    }
    set_data(op.result, std::move(shifted.data_bytes));
}

static std::vector< std::byte > bit_or_vec(std::vector< std::byte > a, std::vector< std::byte > const& b)
{
    if (b.size() > a.size())
    {
        a.resize(b.size(), std::byte{0});
    }
    for (std::size_t i = 0; i < b.size(); ++i)
    {
        std::uint8_t av = (i < a.size()) ? static_cast< std::uint8_t >(a[i]) : 0;
        std::uint8_t bv = static_cast< std::uint8_t >(b[i]);
        a[i] = static_cast< std::byte >(av | bv);
    }
    return a;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_rotate_up const& op)
{
    auto value = consume_local_as_data(op.value);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto result_type = get_local_type(op.result);

    std::size_t bits = get_bit_width_for_type(result_type);
    if (bits == 0)
    {
        set_data(op.result, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes);
    if (bits != 0)
    {
        amt = amt % bits;
    }
    if (amt == 0)
    {
        auto out = truncate_to_bits(std::move(value), bits);
        out.resize(bytes_for_bits(bits), std::byte{0});
        set_data(op.result, std::move(out));
        return;
    }

    auto up = quxlang::bytemath::detail::le_shift_up_raw(value, static_cast< std::size_t >(amt));
    auto down = quxlang::bytemath::detail::le_shift_down_raw(value, static_cast< std::size_t >(bits - amt));

    up = truncate_to_bits(std::move(up), bits);
    down = truncate_to_bits(std::move(down), bits);

    auto combined = bit_or_vec(std::move(up), down);
    combined = truncate_to_bits(std::move(combined), bits);
    combined.resize(bytes_for_bits(bits), std::byte{0});
    set_data(op.result, std::move(combined));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_rotate_down const& op)
{
    auto value = consume_local_as_data(op.value);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto result_type = get_local_type(op.result);

    std::size_t bits = get_bit_width_for_type(result_type);
    if (bits == 0)
    {
        set_data(op.result, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes);
    if (bits != 0)
    {
        amt = amt % bits;
    }
    if (amt == 0)
    {
        auto out = truncate_to_bits(std::move(value), bits);
        out.resize(bytes_for_bits(bits), std::byte{0});
        set_data(op.result, std::move(out));
        return;
    }

    auto down = quxlang::bytemath::detail::le_shift_down_raw(value, static_cast< std::size_t >(amt));
    auto up = quxlang::bytemath::detail::le_shift_up_raw(value, static_cast< std::size_t >(bits - amt));

    down = truncate_to_bits(std::move(down), bits);
    up = truncate_to_bits(std::move(up), bits);

    auto combined = bit_or_vec(std::move(down), up);
    combined = truncate_to_bits(std::move(combined), bits);
    combined.resize(bytes_for_bits(bits), std::byte{0});
    set_data(op.result, std::move(combined));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::bitwise_inverse const& op)
{
    auto value = consume_local_as_data(op.value);
    auto result_type = get_local_type(op.result);
    std::size_t bits = get_bit_width_for_type(result_type);

    bitwise_not_inplace(value);
    value = truncate_to_bits(std::move(value), bits);
    value.resize(bytes_for_bits(bits), std::byte{0});
    set_data(op.result, std::move(value));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_shift_up const& op)
{
    auto target = load_from_reference(op.target, true);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto target_type = remove_ref(get_local_type(op.target));

    std::size_t bits = get_bit_width_for_type(target_type);
    if (bits == 0)
    {
        local_set_data(target, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes);
    bytemath::fixed_int_options opts{};
    opts.bits = bits;
    opts.overflow_undefined = true;

    auto shifted = bytemath::fixed_int_shift_up_le(opts, std::move(target->data), amt);
    if (shifted.result_is_undefined)
    {
        throw constexpr_logic_execution_error("undefined behavior, shift amount overflow");
    }
    local_set_data(target, std::move(shifted.data_bytes));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_shift_down const& op)
{
    auto target = load_from_reference(op.target, true);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto target_type = remove_ref(get_local_type(op.target));

    std::size_t bits = get_bit_width_for_type(target_type);
    if (bits == 0)
    {
        local_set_data(target, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes);
    bytemath::fixed_int_options opts{};
    opts.bits = bits;
    opts.overflow_undefined = true;

    auto shifted = bytemath::fixed_int_shift_down_le(opts, std::move(target->data), amt);
    if (shifted.result_is_undefined)
    {
        throw constexpr_logic_execution_error("undefined behavior, shift amount overflow");
    }
    local_set_data(target, std::move(shifted.data_bytes));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_rotate_up const& op)
{
    auto target = load_from_reference(op.target, true);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto target_type = remove_ref(get_local_type(op.target));

    std::size_t bits = get_bit_width_for_type(target_type);
    if (bits == 0)
    {
        local_set_data(target, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes) % bits;
    if (amt == 0)
    {
        auto out = truncate_to_bits(std::move(target->data), bits);
        out.resize(bytes_for_bits(bits), std::byte{0});
        local_set_data(target, std::move(out));
        return;
    }

    auto value = std::move(target->data);
    auto up = quxlang::bytemath::detail::le_shift_up_raw(value, static_cast< std::size_t >(amt));
    auto down = quxlang::bytemath::detail::le_shift_down_raw(value, static_cast< std::size_t >(bits - amt));
    up = truncate_to_bits(std::move(up), bits);
    down = truncate_to_bits(std::move(down), bits);
    auto combined = bit_or_vec(std::move(up), down);
    combined = truncate_to_bits(std::move(combined), bits);
    combined.resize(bytes_for_bits(bits), std::byte{0});
    local_set_data(target, std::move(combined));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::mut_bitwise_rotate_down const& op)
{
    auto target = load_from_reference(op.target, true);
    auto amount_bytes = consume_local_as_data(op.amount);
    auto target_type = remove_ref(get_local_type(op.target));

    std::size_t bits = get_bit_width_for_type(target_type);
    if (bits == 0)
    {
        local_set_data(target, {});
        return;
    }

    std::uint64_t amt = bytes_to_u64(amount_bytes) % bits;
    if (amt == 0)
    {
        auto out = truncate_to_bits(std::move(target->data), bits);
        out.resize(bytes_for_bits(bits), std::byte{0});
        local_set_data(target, std::move(out));
        return;
    }

    auto value = std::move(target->data);
    auto down = quxlang::bytemath::detail::le_shift_down_raw(value, static_cast< std::size_t >(amt));
    auto up = quxlang::bytemath::detail::le_shift_up_raw(value, static_cast< std::size_t >(bits - amt));
    down = truncate_to_bits(std::move(down), bits);
    up = truncate_to_bits(std::move(up), bits);
    auto combined = bit_or_vec(std::move(down), up);
    combined = truncate_to_bits(std::move(combined), bits);
    combined.resize(bytes_for_bits(bits), std::byte{0});
    local_set_data(target, std::move(combined));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::pcmp_eq const& ceq)
{
    auto ptr1 = load_as_pointer(ceq.a, true);
    auto ptr2 = load_as_pointer(ceq.b, true);

    std::partial_ordering cmp_result = pointer_compare(ptr1, ptr2);
    try
    {
        if (cmp_result == std::partial_ordering::equivalent)
        {
            set_data(ceq.result, {std::byte(1)});
        }
        else
        {
            set_data(ceq.result, {std::byte(0)});
        }
    }
    catch (constexpr_logic_execution_error const& er)
    {
        throw constexpr_logic_execution_error("Error executing <pcmp_eq>: " + std::string(er.what()));
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::pcmp_ge const& cge)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::pcmp_lt const& cge)
{
    auto ptr1 = load_as_pointer(cge.a, true);
    auto ptr2 = load_as_pointer(cge.b, true);

    std::partial_ordering cmp_result = pointer_compare(ptr1, ptr2);
    if (cmp_result == std::partial_ordering::less)
    {
        set_data(cge.result, {std::byte(1)});
    }
    else if (cmp_result == std::partial_ordering::greater || cmp_result == std::partial_ordering::equivalent)
    {
        set_data(cge.result, {std::byte(0)});
    }
    else
    {
        throw constexpr_logic_execution_error("Error executing <pcmp_lt>: pointers are unordered, program behavior is undefined");
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::pcmp_ne const& cne)
{
    auto ptr1 = load_as_pointer(cne.a, true);
    auto ptr2 = load_as_pointer(cne.b, true);

    std::partial_ordering cmp_result = pointer_compare(ptr1, ptr2);
    try
    {
        if (cmp_result == std::partial_ordering::equivalent)
        {
            set_data(cne.result, {std::byte(0)});
        }
        else
        {
            set_data(cne.result, {std::byte(1)});
        }
    }
    catch (constexpr_logic_execution_error const& er)
    {
        throw constexpr_logic_execution_error("Error executing <pcmp_ne>: " + std::string(er.what()));
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::gcmp_eq const& cge)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::gcmp_ge const& cge)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::gcmp_lt const& cge)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::gcmp_ne const& cge)
{
    throw rpnx::unimplemented();
}

quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter()
{
    this->implementation = new ir2_constexpr_interpreter_impl();
}

quxlang::vmir2::ir2_constexpr_interpreter::~ir2_constexpr_interpreter()
{
    delete this->implementation;
    this->implementation = nullptr;
}

void quxlang::vmir2::ir2_constexpr_interpreter::add_class_layout(quxlang::type_symbol name, quxlang::class_layout layout)
{
    this->implementation->class_layouts[name] = std::move(layout);
}

void quxlang::vmir2::ir2_constexpr_interpreter::set_constexpr_result_global_symbol(std::optional< type_symbol > symbol)
{
    this->implementation->constexpr_result_global_symbol = std::move(symbol);
}

void quxlang::vmir2::ir2_constexpr_interpreter::add_constexpr_antestatal_global(type_symbol symbol, type_symbol type, antestatal_value value)
{
    add_constexpr_antestatal_global(std::move(symbol), std::move(type), std::move(value), false);
}

/// Adds an antestatal root with explicit mutability for constexpr localdata/static evaluation.
void quxlang::vmir2::ir2_constexpr_interpreter::add_constexpr_antestatal_global(type_symbol symbol, type_symbol type, antestatal_value value, bool is_mutable)
{
    auto const symbol_copy = symbol;
    auto const type_copy = type;

    this->implementation->constexpr_antestatal_global_types[std::move(symbol)] = std::move(type);
    this->implementation->collect_missing_antestatal_globals(value, type_copy);
    this->implementation->constexpr_antestatal_global_values[symbol_copy] = std::move(value);
    this->implementation->constexpr_antestatal_global_mutable[symbol_copy] = is_mutable;
    this->implementation->missing_antestatal_globals_val.erase(symbol_copy);

    auto root_it = this->implementation->antestatal_global_roots.find(symbol_copy);
    if (root_it != this->implementation->antestatal_global_roots.end() && root_it->second != nullptr)
    {
        this->implementation->initialize_local_from_antestatal_value(root_it->second, type_copy, this->implementation->constexpr_antestatal_global_values.at(symbol_copy));
        if (!is_mutable)
        {
            this->implementation->set_readonly_tree(root_it->second);
        }
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::add_functanoid3(type_symbol addr, functanoid_routine3 func)
{
    auto preload_localdata = [&](auto const& localdata)
    {
        for (auto const& [symbol, entry] : localdata)
        {
            auto type_erased_symbol = type_symbol(symbol);
            this->implementation->constexpr_antestatal_global_types[type_erased_symbol] = entry.type;
            this->implementation->constexpr_antestatal_global_values[type_erased_symbol] = entry.value;
            this->implementation->constexpr_antestatal_global_mutable[type_erased_symbol] = entry.is_mutable;
            this->implementation->missing_antestatal_globals_val.erase(type_erased_symbol);
        }
    };

    auto initialize_localdata = [&](auto const& localdata)
    {
        for (auto const& [symbol, entry] : localdata)
        {
            auto type_erased_symbol = type_symbol(symbol);
            this->implementation->collect_missing_antestatal_globals(entry.value, entry.type);
            auto root_it = this->implementation->antestatal_global_roots.find(type_erased_symbol);
            if (root_it != this->implementation->antestatal_global_roots.end() && root_it->second != nullptr)
            {
                this->implementation->initialize_local_from_antestatal_value(root_it->second, entry.type, entry.value);
                if (!entry.is_mutable)
                {
                    this->implementation->set_readonly_tree(root_it->second);
                }
            }
        }
    };

    preload_localdata(func.static_snapshots);
    initialize_localdata(func.static_snapshots);

    // Track missing dtors
    for (auto const& dtor : func.non_trivial_dtors)
    {
        auto dtor_func = dtor.second;
        if (!this->implementation->functanoids3.contains(dtor_func))
        {
            this->implementation->missing_functanoids_val.insert(dtor_func);
        }
    }
    // Track missing invoked functions
    for (auto const& block : func.blocks)
    {
        for (auto const& instr : block.instructions)
        {
            if (typeis< vmir2::invoke >(instr))
            {
                auto const& inv = instr.get_as< vmir2::invoke >();
                auto called_func = inv.what;
                if (!this->implementation->functanoids3.contains(called_func))
                {
                    this->implementation->missing_functanoids_val.insert(called_func);
                }
            }
            else if (typeis< vmir2::get_procedure_ptr >(instr))
            {
                auto const& gpp = instr.get_as< vmir2::get_procedure_ptr >();
                if (!this->implementation->functanoids3.contains(gpp.routine))
                {
                    this->implementation->missing_functanoids_val.insert(gpp.routine);
                }
            }
            else if (typeis< vmir2::interface_init >(instr))
            {
                vmir2::interface_init const& init = instr.get_as< vmir2::interface_init >();
                for (std::pair< interface_slot_key const, type_symbol > const& entry : init.functions)
                {
                    type_symbol const& routine = entry.second;
                    if (!this->implementation->functanoids3.contains(routine))
                    {
                        this->implementation->missing_functanoids_val.insert(routine);
                    }
                }
            }
            else if (typeis< vmir2::interface_invoke >(instr))
            {
                vmir2::interface_invoke const& inv = instr.get_as< vmir2::interface_invoke >();
                if (inv.default_function.has_value() && !this->implementation->functanoids3.contains(*inv.default_function))
                {
                    this->implementation->missing_functanoids_val.insert(*inv.default_function);
                }
            }
            else if (typeis< vmir2::get_antestatal_ref >(instr))
            {
                auto const& gar = instr.get_as< vmir2::get_antestatal_ref >();
                if (this->implementation->has_constexpr_antestatal_global(gar.symbol))
                {
                    continue;
                }
                if (typeis< static_local_ref >(gar.symbol) || typeis< static_snapshot_ref >(gar.symbol))
                {
                    throw compiler_bug("missing static localdata for " + quxlang::to_string(gar.symbol));
                }
                this->implementation->mark_missing_antestatal_global(gar.symbol);
            }
        }
    }
    this->implementation->functanoids3[addr] = std::move(func);
    this->implementation->missing_functanoids_val.erase(addr);
}

void quxlang::vmir2::ir2_constexpr_interpreter::set_source_index(source_index source_index)
{
    this->implementation->printer_source_index = std::move(source_index);
}

void quxlang::vmir2::ir2_constexpr_interpreter::exec(type_symbol func)
{
    this->implementation->call_func(func, {});
    this->implementation->exec();
}
void quxlang::vmir2::ir2_constexpr_interpreter::exec3(type_symbol func)
{
    this->implementation->exec_mode = 2;
    auto func_cow = cow< type_symbol >(func);
    this->implementation->call_func(func_cow, {});
    this->implementation->exec3();
}
bool quxlang::vmir2::ir2_constexpr_interpreter::get_cr_bool()
{
    assert(this->implementation->constexpr_result_v.size() == 1);
    return this->implementation->constexpr_result_v == std::vector< std::byte >{std::byte(1)};
}
std::uint64_t quxlang::vmir2::ir2_constexpr_interpreter::get_cr_u64()
{
    std::uint64_t result = 0;
    if (this->implementation->constexpr_result_v.size() != 8)
    {
        throw std::logic_error("expected uint64");
    }
    for (std::size_t i = 0; i < 8; i++)
    {
        result |= (static_cast< std::uint64_t >(static_cast< std::uint8_t >(this->implementation->constexpr_result_v[i])) << (i * 8));
    }
    return result;
}
quxlang::constexpr_result quxlang::vmir2::ir2_constexpr_interpreter::get_cr_value()
{
    return constexpr_result{.type = void_type{}, .value = this->implementation->constexpr_result_v};
}

quxlang::antestatal_value quxlang::vmir2::ir2_constexpr_interpreter::get_cr_antestatal_value()
{
    if (this->implementation->constexpr_result_antestatal_values.contains(0))
    {
        return this->implementation->constexpr_result_antestatal_values.at(0);
    }
    if (!this->implementation->constexpr_result_antestatal.has_value())
    {
        throw std::logic_error("expected antestatal constexpr result");
    }
    return *this->implementation->constexpr_result_antestatal;
}

/// Returns every antestatal result materialized by constexpr_set_result2, keyed by result ID.
std::map< std::uint64_t, quxlang::antestatal_value > quxlang::vmir2::ir2_constexpr_interpreter::get_cr_antestatal_values()
{
    return this->implementation->constexpr_result_antestatal_values;
}

std::map< std::uint64_t, quxlang::constexpr_serialoid > quxlang::vmir2::ir2_constexpr_interpreter::get_cr_serialoid_values()
{
    return this->implementation->constexpr_result_serialoid_values;
}

std::set< quxlang::type_symbol > const& quxlang::vmir2::ir2_constexpr_interpreter::missing_functanoids()
{
    return this->implementation->missing_functanoids_val;
}

std::set< quxlang::type_symbol > const& quxlang::vmir2::ir2_constexpr_interpreter::missing_antestatal_globals()
{
    return this->implementation->missing_antestatal_globals_val;
}

std::vector< std::byte > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::consume_local_as_data(vmir2::local_index slot)
{
    auto& frame = get_current_frame();

    auto it = frame.local_values.find(slot);
    if (it == frame.local_values.end() || !it->second)
    {
        throw constexpr_logic_execution_error("Error in [consume_data] substep: slot does not exist");
    }

    auto local = it->second;

    // Move the data out of the local
    return local_consume_data(local);
}

std::vector< std::byte > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::copy_data(vmir2::local_index slot)
{
    auto& frame = get_current_frame();

    auto it = frame.local_values.find(slot);
    if (it == frame.local_values.end() || !it->second)
    {
        throw constexpr_logic_execution_error("Error in [copy_data] substep: slot does not exist");
    }

    // Copy the data from the local
    std::vector< std::byte > data = it->second->data;

    return data;
}
std::vector< std::byte > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local_consume_data(std::shared_ptr< local > local_value)
{
    std::vector< std::byte > data = std::move(local_value->data);
    local_value->stage = slot_stage::dead;
    if (!local_value->member_of.has_value())
    {
        local_value->storage_initiated = false;
    }
    return data;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local_set_data(std::shared_ptr< local > local_value, std::vector< std::byte > data)
{
    assert(local_value != nullptr);
    if (local_value->readonly)
    {
        throw constexpr_logic_execution_error("attempted to modify read-only constexpr object");
    }
    // TODO: Consider check if value already alive or not.
    local_value->data = std::move(data);

    begin_lifetime(local_value);
}
bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_is_nullptr(pointer_impl ptr)
{
    return !ptr.pointer_target.has_value() && !ptr.one_past_the_end.has_value() && ptr.invalidated == false;
}
bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_is_one_past_the_end(pointer_impl ptr)
{
    return !ptr.pointer_target.has_value() && ptr.one_past_the_end.has_value() && ptr.invalidated == false;
}
bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_invalidated(pointer_impl ptr)
{
    auto allocation_index_for_local = [&](std::shared_ptr< local > target) -> std::optional< std::size_t >
    {
        while (target != nullptr)
        {
            auto direct_it = constexpr_allocation_lookup.find(target.get());
            if (direct_it != constexpr_allocation_lookup.end())
            {
                return direct_it->second;
            }

            if (target->storage_owner.has_value())
            {
                auto owner = target->storage_owner.value().lock();
                if (owner != nullptr)
                {
                    target = std::move(owner);
                    continue;
                }
            }

            if (target->member_of.has_value())
            {
                auto owner = target->member_of.value().lock();
                if (owner != nullptr)
                {
                    target = std::move(owner);
                    continue;
                }
            }

            break;
        }
        return std::nullopt;
    };

    auto points_into_freed_allocation = [&](std::shared_ptr< local > const& target) -> bool
    {
        auto allocation_index = allocation_index_for_local(target);
        return allocation_index.has_value() && constexpr_allocations.at(*allocation_index).freed;
    };

    if (ptr.invalidated)
    {
        return true;
    }

    if (ptr.pointer_target.has_value())
    {
        if (ptr.pointer_target.value().expired())
        {
            return true;
        }
        if (!ptr.pointer_target.value().lock()->storage_initiated)
        {
            return true;
        }
        if (points_into_freed_allocation(ptr.pointer_target.value().lock()))
        {
            return true;
        }
    }

    if (ptr.one_past_the_end.has_value())
    {
        if (ptr.one_past_the_end.value().expired())
        {
            return true;
        }
        if (!ptr.one_past_the_end.value().lock()->storage_initiated)
        {
            return true;
        }
        if (points_into_freed_allocation(ptr.one_past_the_end.value().lock()))
        {
            return true;
        }
    }

    return false;
}
bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_targets_object(pointer_impl ptr)
{
    assert(!pointer_invalidated(ptr));
    if (pointer_is_nullptr(ptr) || pointer_is_one_past_the_end(ptr))
    {
        return false;
    }
    return true;
}
std::optional< std::weak_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_memberof(pointer_impl input)
{
    assert(!pointer_is_nullptr(input));

    if (pointer_is_one_past_the_end(input))
    {
        // One-past-the-end pointer
        auto arry = input.one_past_the_end.value();
        return arry;
    }
    else
    {
        assert(!input.one_past_the_end.has_value());

        auto target_object = input.pointer_target.value().lock();

        if (target_object == nullptr)
        {
            throw constexpr_logic_execution_error("pointer arithmetic on pointer to already invalidated storage is UB");
        }

        return target_object->member_of;
    }
}
std::int64_t quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_offset_in_array(std::shared_ptr< local > array, pointer_impl ptr)
{
    assert(!pointer_is_nullptr(ptr));
    assert(array != nullptr);
    assert(pointer_memberof(ptr).value().lock() == array);
    assert(!pointer_invalidated(ptr));

    if (pointer_is_one_past_the_end(ptr))
    {
        return static_cast< std::int64_t >(array->array_members.size());
    }

    assert(ptr.pointer_target.has_value());

    auto target = ptr.pointer_target.value().lock();
    if (!target)
    {
        throw compiler_bug("how");
    }

    for (std::size_t i = 0; i < array->array_members.size(); i++)
    {
        if (array->array_members[i] == target)
        {
            return static_cast< std::int64_t >(i);
        }
    }

    throw compiler_bug("pointer target not found in array");
}

quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_impl quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_arith(pointer_impl input, std::int64_t offset, type_symbol type)
{
    std::shared_ptr< local > arry = nullptr;

    if (pointer_is_nullptr(input))
    {
        // NULLPTR
        if (offset == 0)
        {
            // Offset of +- 0 on nullptr is valid and produces a null pointer
            return input;
        }

        throw constexpr_logic_execution_error("Error executing pointer arithmetic on null pointer");
    }

    if (pointer_invalidated(input))
    {
        if (offset == 0)
        {
            // Offset of +- 0 on invalidated pointer is valid and produces the same invalidated pointer
            return input;
        }

        throw constexpr_logic_execution_error("Error executing pointer arithmetic on invalidated pointer");
    }

    // if not nullptr, we must be a member of an array:
    arry = pointer_memberof(input).value().lock();

    // TODO: Verify this calculation doesn't overflow
    std::int64_t new_position = pointer_offset_in_array(arry, input) + offset;

    if (new_position < 0)
    {
        throw constexpr_logic_execution_error("pointer arithmetic: constructing pointer out of bounds is UB");
    }
    else if (new_position == arry->array_members.size())
    {
        pointer_impl output;
        output.one_past_the_end = arry;
        output.pointer_target = std::nullopt;
        return output;
    }
    else if (new_position > arry->array_members.size())
    {
        throw constexpr_logic_execution_error("pointer arithmetic: constructing pointer out of bounds is UB");
    }
    else
    {
        pointer_impl output;
        output.one_past_the_end = std::nullopt;
        output.pointer_target = arry->array_members[new_position];
        return output;
    }
}
std::partial_ordering quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_compare(pointer_impl a, pointer_impl b)
{

    if (pointer_is_nullptr(a) != pointer_is_nullptr(b))
    {
        // TODO: Globally ordered pointers
        return std::partial_ordering::unordered;
    }

    // The only valid use of an invalidated pointer is a comparison with nullptr, which is checked above,
    // all remaining comparisons are UB.
    if (pointer_invalidated(a) || pointer_invalidated(b))
    {
        throw constexpr_logic_execution_error("comparison of an invalidated pointer with a value other than NULLPTR is undefined behavior");
    }

    auto p1 = pointer_memberof(a).value().lock();
    auto p2 = pointer_memberof(b).value().lock();
    if (p1 != p2)
    {
        // TODO: Implement global pointer ordering here where requested.
        return std::partial_ordering::unordered;
    }

    auto off1 = pointer_offset_in_array(p1, a);
    auto off2 = pointer_offset_in_array(p2, b);
    if (off1 < off2)
    {
        return std::partial_ordering::less;
    }
    else if (off1 > off2)
    {
        return std::partial_ordering::greater;
    }
    else
    {
        return std::partial_ordering::equivalent;
    }
}

std::uint64_t quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::consume_u64(local_index slot)
{
    auto data = consume_local_as_data(slot);
    if (data.size() != 8)
    {
        throw std::logic_error("expected uint64");
    }
    std::uint64_t result = 0;
    for (std::size_t i = 0; i < 8; i++)
    {
        result |= (static_cast< std::uint64_t >(static_cast< std::uint8_t >(data[i])) << (i * 8));
    }
    return result;
}
std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::consume_local(local_index slot)
{
    // TODO: Add lifetime checks here
    std::shared_ptr< local > result = nullptr;

    result = get_current_frame().local_values[slot];

    // Mark value as consumed (dead)
    end_lifetime(result);
    // Align storage validity semantics with state_engine.consume():
    // storage_valid becomes true only if this slot is a delegate or storage-backed,
    // otherwise consuming invalidates the storage.
    result->storage_initiated = result->member_of.has_value() || result->storage_owner.has_value();

    // Remove from frame to reflect consumption
    get_current_frame().local_values[slot] = nullptr;

    return result;
}

quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::interface_object quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::load_interface_object(local_index slot, bool consume)
{
    std::shared_ptr< local > local_ptr;
    auto& frame_slot = get_current_frame().local_values.at(slot);
    if (!frame_slot || !frame_slot->alive())
    {
        throw constexpr_logic_execution_error("Error executing interface instruction: interface handle is not alive");
    }

    if (consume)
    {
        local_ptr = consume_local(slot);
    }
    else
    {
        local_ptr = frame_slot;
    }

    type_symbol slot_type = get_local_type(slot);
    if (is_ref(slot_type))
    {
        if (!local_ptr->ref.has_value() || !local_ptr->ref->pointer_target.has_value())
        {
            throw constexpr_logic_execution_error("Error executing interface instruction: reference has no target");
        }
        if (pointer_invalidated(*local_ptr->ref))
        {
            throw constexpr_logic_execution_error("Error executing interface instruction: reference target is invalidated");
        }
        std::shared_ptr< local > target = local_ptr->ref->pointer_target->lock();
        if (!target || !target->alive())
        {
            throw constexpr_logic_execution_error("Error executing interface instruction: reference target is not alive");
        }
        if (!target->interface_value.has_value())
        {
            throw constexpr_logic_execution_error("Error executing interface instruction: referenced object is not an interface handle");
        }
        return *target->interface_value;
    }

    if (!local_ptr->interface_value.has_value())
    {
        throw constexpr_logic_execution_error("Error executing interface instruction: object is not an interface handle");
    }
    return *local_ptr->interface_value;
}

uint64_t quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::alloc_object_id()
{
    return next_object_id++;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::set_data(local_index slot, std::vector< std::byte > data)
{
    auto& frame = get_current_frame();

    auto& local_ptr = frame.local_values[slot];
    if (local_ptr == nullptr)
    {
        create_local_value(slot, false);
    }
    auto& local = *local_ptr;
    local.data = std::move(data);
    begin_lifetime(local_ptr);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::output_bool(local_index slot_index, bool value)
{
    auto& frame = get_current_frame();

    auto slot_type = frame_slot_data_type(slot_index);
    if (slot_type != bool_type{})
    {
        throw invalid_instruction_error("Error in [output_bool]: slot is not a boolean");
    }

    auto slot = create_local_value(slot_index, false);

    local_set_data(slot, {value ? std::byte(1) : std::byte(0)});
}
quxlang::vmir2::slot_state quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_current_slot_state(std::size_t frame_index, local_index slot_index)
{
    quxlang::vmir2::slot_state result;

    auto& frame_object = this->stack.at(frame_index);

    auto& slot_ptr = frame_object.local_values[slot_index];

    if (!frame_object.local_values.contains(slot_index) || slot_ptr == nullptr)
    {
        return result;
    }

    auto& slot_object = *slot_ptr;

    result.stage = slot_object.stage;
    result.storage_valid = slot_object.storage_initiated;

    if (slot_object.member_of.has_value())
    {
        auto idx = get_index(frame_index, slot_object.member_of.value().lock());
        if (idx != 0)
        {

            result.delegate_of = idx;
        }
    }
    result.destroy_delegate = slot_object.storage_destroy_delegate;

    for (auto [idx, local] : frame_object.local_values)
    {
        if (local == nullptr)
        {
            continue;
        }
        if (local->initializer_of.has_value() && local->initializer_of.value().lock() == slot_ptr)
        {
            if (!result.delegates)
            {
                result.delegates = invocation_args{};
            }

            result.delegates.value().named["INIT"] = get_index(frame_index, local);
        }
    }

    for (auto [name, local] : slot_object.struct_members)
    {
        if (local == nullptr)
        {
            continue;
        }

        auto idx = get_index(frame_index, local);
        if (idx != 0)
        {
            if (!result.delegates)
            {
                result.delegates = invocation_args{};
            }
            result.delegates.value().named[name] = idx;
        }
    }

    // auto delgs = slot_object.delegates;

    // result.delegates = delgs;

    if (slot_object.array_init_member_of.has_value())
    {

        auto loc = slot_object.array_init_member_of.value().lock();
        auto idx = get_index(frame_index, loc);
        if (idx != 0)
        {
            result.array_delegate_of_initializer = idx;
            result.delegate_of = std::nullopt;
        }
    }

    if (slot_object.initializer_of.has_value())
    {
        auto idx = get_index(frame_index, slot_object.initializer_of.value().lock());
        result.delegate_of = idx;
    }
    return result;
}
quxlang::vmir2::state_map quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_expected_state_map_preexec(std::size_t frame_index, std::size_t block_index, std::size_t instruction_index)
{
    throw compiler_bug("removed");
}

quxlang::vmir2::state_map quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_expected_state_map_preexec3(std::size_t frame_index, std::size_t block_index, std::size_t instruction_index)
{
    vmir2::functanoid_routine3 const& func_ir = stack.at(frame_index).ir3.read();
    state_map state = func_ir.blocks.at(block_index).entry_state;

    auto const& instructions = func_ir.blocks.at(block_index).instructions;

    for (std::size_t i = 0; i < instruction_index && i < instructions.size(); i++)
    {
        auto const& instr = instructions.at(i);
        codegen_state_engine(state, func_ir.local_types, func_ir.parameters).apply(instr);
    }

    for (local_index i(0); i < func_ir.local_types.size(); i++)
    {
        state[i];
    }

    return state;
}

quxlang::vmir2::state_map quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_current_state_map(std::size_t frame_index)
{
    throw compiler_bug("removed");
}

quxlang::vmir2::state_map quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_current_state_map3(std::size_t frame_index)
{
    auto const& current_frame = stack.at(frame_index);
    auto const& func_ir = current_frame.ir3.read();

    state_map result;

    for (local_index i(0); i < func_ir.local_types.size(); i++)
    {
        result[i] = get_current_slot_state(frame_index, i);
    }

    return result;
}

quxlang::vmir2::slot_state quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_expected_slot_state_postexec(std::size_t frame_index, std::size_t slot_index, std::size_t instruction_index)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::require_valid_input_precondition(local_index slot)
{
    assert(get_current_frame().local_values.contains(slot));
    assert(get_current_frame().local_values[slot] != nullptr);
    assert(get_current_frame().local_values[slot]->alive());
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::require_valid_output_precondition(local_index slot)
{
    assert(!get_current_frame().local_values.contains(slot) || get_current_frame().local_values[slot] == nullptr || !get_current_frame().local_values[slot]->alive());
}
quxlang::type_symbol quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::frame_slot_data_type(vmir2::local_index slot)
{
    return get_local_type(slot);
}
quxlang::vmir2::local_index quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_index(std::size_t frame_index, std::shared_ptr< local > local_value)
{
    auto& frame = stack.at(frame_index);

    for (auto const& [index, slot] : frame.local_values)
    {
        if (slot == local_value)
        {
            return local_index(index);
        }
    }

    return local_index(0);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(quxlang::vmir2::defer_nontrivial_dtor const& dntd)
{
    auto& frame = get_current_frame();

    auto& slot = frame.local_values[dntd.on_value];

    if (!slot)
    {
        throw compiler_bug("Error in [defer_nontrivial_dtor]: storage not allocated");
    }

    if (dntd.args.named.at("THIS") != dntd.on_value)
    {
        throw compiler_bug("Error in [defer_nontrivial_dtor]: THIS argument does not match target");
    }

    slot->dtor = dtor_spec{.func = dntd.func, .args = dntd.args};
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::struct_init_start const& sdn)
{
    auto& frame = get_current_frame();

    auto& slot = frame.local_values[sdn.on_value];

    if (slot == nullptr)
    {
        slot = create_local_value(sdn.on_value, false);
    }

    slot->stage = slot_stage::partial;
    slot->storage_initiated = true;
    slot->delegates = sdn.fields;

    // TODO: May need to implement SDN on arrays?

    assert(sdn.fields.positional.size() == 0);

    for (auto& [name, index] : sdn.fields.named)
    {
        auto const& field_type = get_local_type(index);
        auto& field_slot = frame.local_values[index];
        if (field_slot != nullptr)
        {
            throw compiler_bug("it shouldn't be possible to have a field slot pre-initialized, something is wrong with codegen");
        }

        create_local_value(index, false);

        slot->struct_members[name] = field_slot;
        field_slot->member_of = slot;
    }

    // throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::struct_init_finish const& scn)
{
    auto& frame = get_current_frame();

    auto& slot = frame.local_values[scn.on_value];

    if (slot == nullptr)
    {
        throw compiler_bug("this shouldn't be possible");
    }

    begin_lifetime(slot);

    if (!slot->delegates.has_value())
    {
        throw compiler_bug("struct_complete_new: delegates not initialized");
    }
    // TODO: May need to implement SDN on arrays?

    for (auto& [name, dlg] : slot->delegates.value().named)
    {
        frame.local_values.erase(dlg);
    }

    slot->delegates = std::nullopt;

    // throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::copy_reference const& cpr)
{
    auto ptr = get_pointer_to(current_frame_index(), cpr.from_index);

    auto ref_type = get_local_type(cpr.to_index);
    if (!is_ref(ref_type))
    {
        throw compiler_bug("Attempt to make reference by non-reference type");
    }

    if (get_current_frame().local_values[cpr.to_index])
    {
        throw compiler_bug("Attempt to overwrite reference (A)");
    }

    auto local_ptr = output_local(cpr.to_index);

    local_ptr->ref = ptr;

    return;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::destroy const& dst)
{
    auto& frame = get_current_frame();
    auto& local_ptr = frame.local_values.at(dst.of);
    if (local_ptr == nullptr || !local_ptr->alive())
    {
        throw constexpr_logic_execution_error("destroy expects a live local");
    }

    auto slot_type = get_local_type(dst.of);
    if (frame.ir3->non_trivial_dtors.contains(slot_type))
    {
        auto dtor = frame.ir3->non_trivial_dtors.at(slot_type);
        call_func(dtor, {.named = {{"THIS", dst.of}}});
        return;
    }

    end_lifetime(local_ptr);
    local_ptr = nullptr;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::end_lifetime const& elt)
{
    auto& frame = get_current_frame();

    auto& local_ptr = frame.local_values.at(elt.of);
    auto slot_type = get_local_type(elt.of);
    if (local_ptr != nullptr && (typeis< storage >(slot_type) || typeis< aligned_storage >(slot_type)) && local_ptr->storage_active_type.has_value())
    {
        throw constexpr_logic_execution_error("storage lifetime ended while containing an active object");
    }
    end_lifetime(local_ptr);
    local_ptr = nullptr;
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::create_local_value(vmir2::local_index local_idx, bool set_alive)
{
    auto& frame = get_current_frame();

    auto result_type = frame.ir3->local_types.at(local_idx).type;

    if (frame.local_values[local_idx] == nullptr)
    {
        frame.local_values[local_idx] = create_object_skeleton(result_type);
    }

    frame.local_values[local_idx]->storage_initiated = true;

    if (set_alive)
    {
        begin_lifetime(frame.local_values[local_idx]);
    }

    return frame.local_values[local_idx];
}
std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::create_object_skeleton(type_symbol type)
{
    auto val = std::make_shared< local >();
    val->object_id = next_object_id++;
    auto& r_data = val->data;
    r_data.resize(get_type_size(type));
    std::fill(r_data.begin(), r_data.end(), std::byte(0));
    val->stage = slot_stage::dead;
    return val;
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::create_object(type_symbol type)
{
    auto obj = create_object_skeleton(type);
    init_storage(obj, type);
    assert(obj->stage == slot_stage::dead);
    return obj;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::init_storage(std::shared_ptr< local > local_value, type_symbol type)
{
    local_value->storage_initiated = true;
    local_value->stage = slot_stage::dead;
    local_value->storage_alignment = get_type_alignment(type);
    local_value->stored_object = nullptr;
    local_value->storage_active_type = std::nullopt;
    local_value->storage_projection_type = std::nullopt;
    local_value->storage_destroy_delegate = false;
    local_value->procedure = std::nullopt;

    if (typeis< attached_type_reference >(type))
    {
        attached_type_reference const& attached = as< attached_type_reference >(type);
        if (typeis< void_type >(attached.carrying_type))
        {
            return;
        }
        init_storage(local_value, attached.carrying_type);
        return;
    }

    type.match< readonly_constant >(
        [&](readonly_constant const& rc)
        {
            auto byteptr_type = ptrref_type{.target = int_type{.bits = 8, .has_sign = false}, .ptr_class = pointer_class::array, .qual = qualifier::constant};
            local_value->struct_members["__start"] = create_object(byteptr_type);
            local_value->struct_members["__start"]->member_of = local_value;
            local_value->struct_members["__end"] = create_object(byteptr_type);
            local_value->struct_members["__end"]->member_of = local_value;
        });

    if (class_layouts.contains(type))
    {
        class_layout const& layout = class_layouts.at(type);

        for (auto const& field : layout.fields)
        {
            if (typeis< attached_type_reference >(field.type) && typeis< void_type >(as< attached_type_reference >(field.type).carrying_type))
            {
                continue;
            }

            auto const& name = field.name;
            if (local_value->struct_members[name] == nullptr)
            {
                local_value->struct_members[name] = std::make_shared< local >();
            }

            local_value->struct_members[name]->member_of = local_value;
            init_storage(local_value->struct_members[name], field.type);
        }
    }

    type.match< array_type >(
        [&](array_type const& array_type)
        {
            if (!array_type.element_count.match< expression_numeric_literal >(
                    [&](expression_numeric_literal const& literal)
                    {
                        std::uint64_t element_count = parsers::str_to_int< std::uint64_t >(literal.value);
                        local_value->array_members.resize(element_count);
                        for (std::size_t i = 0; i < element_count; i++)
                        {
                            if (local_value->array_members.at(i) == nullptr)
                            {
                                local_value->array_members.at(i) = create_object(array_type.element_type);
                            }
                            local_value->array_members.at(i)->member_of = local_value;
                            init_storage(local_value->array_members.at(i), array_type.element_type);
                        }
                    }))
            {
                // The expression must have been resolved to a single numeric literal at this point,
                // types like `[4 + 5]I32` should have been reduced to `[9]I32` before adding to IR.
                throw std::logic_error("unresolved array type");
            }
        });

    if (get_type_size(type) > 0)
    {
        local_value->data.resize(get_type_size(type));
        std::fill(local_value->data.begin(), local_value->data.end(), std::byte(0));
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::begin_lifetime_tree(std::shared_ptr< local > const& object)
{
    if (!object)
    {
        throw compiler_bug("begin_lifetime_tree on null object");
    }

    for (auto const& [_, member] : object->struct_members)
    {
        begin_lifetime_tree(member);
    }
    for (auto const& member : object->array_members)
    {
        begin_lifetime_tree(member);
    }
    if (object->stored_object)
    {
        begin_lifetime_tree(object->stored_object);
    }

    begin_lifetime(object);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::set_readonly_tree(std::shared_ptr< local > const& object)
{
    if (!object)
    {
        return;
    }

    object->readonly = true;

    for (auto const& [_, member] : object->struct_members)
    {
        set_readonly_tree(member);
    }
    for (auto const& member : object->array_members)
    {
        set_readonly_tree(member);
    }
    if (object->stored_object)
    {
        set_readonly_tree(object->stored_object);
    }
}

bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::has_constexpr_antestatal_global(type_symbol const& symbol) const
{
    return constexpr_antestatal_global_values.contains(symbol);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::mark_missing_antestatal_global(type_symbol const& symbol)
{
    if (constexpr_result_global_symbol.has_value() && *constexpr_result_global_symbol == symbol)
    {
        return;
    }
    if (has_constexpr_antestatal_global(symbol))
    {
        return;
    }

    missing_antestatal_globals_val.insert(symbol);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::collect_missing_antestatal_globals(antestatal_access const& access, std::optional< type_symbol > type)
{
    if (typeis< antestatal_access_global >(access))
    {
        mark_missing_antestatal_global(as< antestatal_access_global >(access).symbol);
        return;
    }

    if (typeis< antestatal_access_field >(access))
    {
        auto const& field = as< antestatal_access_field >(access);
        collect_missing_antestatal_globals(field.object);
        return;
    }

    if (typeis< antestatal_access_array_element >(access))
    {
        auto const& array_element = as< antestatal_access_array_element >(access);
        collect_missing_antestatal_globals(array_element.array, std::move(type));
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::collect_missing_antestatal_globals(antestatal_value const& value, std::optional< type_symbol > type)
{
    if (type.has_value())
    {
        if (typeis< nvalue_slot >(*type))
        {
            type = as< nvalue_slot >(*type).target;
        }
        else if (typeis< dvalue_slot >(*type))
        {
            type = as< dvalue_slot >(*type).target;
        }
    }

    if (typeis< antestatal_interface >(value))
    {
        antestatal_interface const& interface_value = as< antestatal_interface >(value);
        for (std::pair< interface_slot_key const, type_symbol > const& function : interface_value.functions)
        {
            if (!functanoids3.contains(function.second))
            {
                missing_functanoids_val.insert(function.second);
            }
        }
        return;
    }

    if (typeis< antestatal_ptrref >(value))
    {
        if (type.has_value() && typeis< ptrref_type >(*type) && typeis< procedure_type >(as< ptrref_type >(*type).target))
        {
            auto const& access = as< antestatal_ptrref >(value).target;
            if (typeis< antestatal_access_global >(access))
            {
                auto routine = as< antestatal_access_global >(access).symbol;
                if (!functanoids3.contains(routine))
                {
                    missing_functanoids_val.insert(std::move(routine));
                }
            }
            return;
        }

        collect_missing_antestatal_globals(as< antestatal_ptrref >(value).target, std::move(type));
        return;
    }

    if (typeis< antestatal_array >(value))
    {
        std::optional< type_symbol > element_type;
        if (type.has_value() && typeis< array_type >(*type))
        {
            element_type = as< array_type >(*type).element_type;
        }

        for (auto const& element : as< antestatal_array >(value).elements)
        {
            collect_missing_antestatal_globals(element, element_type);
        }
        return;
    }

    if (typeis< antestatal_struct >(value))
    {
        auto const& fields = as< antestatal_struct >(value).fields;
        if (type.has_value() && typeis< readonly_constant >(*type))
        {
            auto byte_ptr_type = ptrref_type{.target = byte_type{}, .ptr_class = pointer_class::array, .qual = qualifier::constant};
            for (auto const& [_, field_value] : fields)
            {
                collect_missing_antestatal_globals(field_value, byte_ptr_type);
            }
            return;
        }

        if (type.has_value() && class_layouts.contains(*type))
        {
            auto const& layout = class_layouts.at(*type);
            for (auto const& field : layout.fields)
            {
                type_symbol field_type = field.type;
                if (typeis< attached_type_reference >(field_type))
                {
                    attached_type_reference const& attached = as< attached_type_reference >(field_type);
                    if (typeis< void_type >(attached.carrying_type))
                    {
                        continue;
                    }
                    field_type = attached.carrying_type;
                }
                auto value_it = fields.find(field.name);
                if (value_it != fields.end())
                {
                    collect_missing_antestatal_globals(value_it->second, field_type);
                }
            }
            return;
        }

        for (auto const& [_, field_value] : fields)
        {
            collect_missing_antestatal_globals(field_value);
        }
    }
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::load_from_reference(local_index local_idx, bool consume)
{
    auto& frame = get_current_frame();
    auto& local_ptr = frame.local_values[local_idx];

    if (local_ptr == nullptr)
    {
        throw invalid_instruction_transition_error("Error in [load_from_reference]: slot not allocated");
    }

    if (!local_ptr->alive())
    {
        throw invalid_instruction_transition_error("Error in [load_from_reference]: slot not alive");
    }

    auto ptr_ref = local_ptr->ref;
    if (consume)
    {
        end_lifetime(local_ptr);
        local_ptr = nullptr;
    }

    if (!ptr_ref.has_value())
    {
        throw constexpr_logic_execution_error("nullptr dereference");
    }
    if (pointer_invalidated(*ptr_ref))
    {
        throw constexpr_logic_execution_error("dereferencing invalidated pointer or reference");
    }

    auto ptr_target = ptr_ref.value().pointer_target.value().lock();

    if (!ptr_target)
    {
        throw constexpr_logic_execution_error("dereferencing invalidated pointer or reference");
    }
    if (!ptr_target->alive() && access_to_antestatal_local(ptr_target).has_value())
    {
        throw constexpr_logic_execution_error("cannot read symbolic antestatal static during constexpr evaluation");
    }

    return ptr_target;
}
quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_impl quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::load_as_pointer(local_index slot, bool consume)
{
    auto& frame = get_current_frame();
    std::shared_ptr< local > local_ptr;

    if (consume)
    {
        local_ptr = consume_local(slot);
    }
    else
    {
        local_ptr = frame.local_values[slot];
    }

    auto ptr_ref = local_ptr->ref;
    if (consume)
    {
        local_ptr->ref = std::nullopt;
    }

    if (!ptr_ref.has_value())
    {
        throw constexpr_logic_execution_error("uninitialized dereference");
    }

    return ptr_ref.value();
}
quxlang::type_symbol quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::load_indirect_callable_symbol(local_index slot, bool consume)
{
    auto callable_type = remove_ref(get_local_type(slot));

    auto load_procedure_symbol_from_local = [](std::shared_ptr< local > const& proc_local) -> std::optional< type_symbol >
    {
        if (!proc_local)
        {
            return std::nullopt;
        }
        if (!proc_local->procedure.has_value())
        {
            return std::nullopt;
        }

        auto const& proc_symbol = *proc_local->procedure;
        if (typeis< submember >(proc_symbol))
        {
            return as< submember >(proc_symbol).of;
        }
        return proc_symbol;
    };

    auto load_procedure_symbol_from_pointer_target = [&](std::shared_ptr< local > const& ptr_like_local) -> std::optional< type_symbol >
    {
        if (!ptr_like_local || !ptr_like_local->ref.has_value() || !ptr_like_local->ref->pointer_target.has_value())
        {
            return std::nullopt;
        }

        auto target = ptr_like_local->ref->pointer_target->lock();
        return load_procedure_symbol_from_local(target);
    };

    if (typeis< procedure_type >(callable_type))
    {
        std::shared_ptr< local > local_ptr;
        if (consume)
        {
            local_ptr = consume_local(slot);
        }
        else
        {
            local_ptr = get_current_frame().local_values.at(slot);
        }

        if (auto direct = load_procedure_symbol_from_local(local_ptr))
        {
            return *direct;
        }
        if (local_ptr && local_ptr->ref.has_value() && local_ptr->ref->pointer_target.has_value())
        {
            auto target = local_ptr->ref->pointer_target->lock();
            if (auto indirect = load_procedure_symbol_from_local(target))
            {
                return *indirect;
            }
        }
    }
    else if (typeis< ptrref_type >(callable_type))
    {
        auto const& ptr = as< ptrref_type >(callable_type);
        if (ptr.ptr_class == pointer_class::instance && typeis< procedure_type >(ptr.target))
        {
            auto proc_ptr = load_as_pointer(slot, consume);
            auto target = proc_ptr.pointer_target.value().lock();
            if (auto indirect = load_procedure_symbol_from_local(target))
            {
                return *indirect;
            }
            if (auto indirect = load_procedure_symbol_from_pointer_target(target))
            {
                return *indirect;
            }
        }
    }

    throw constexpr_logic_execution_error("invoke_indirect requires a valid procedure reference or pointer");
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::store_as_reference(local_index slot, std::shared_ptr< local > value)
{
    auto local_ptr = output_local(slot);
    local_ptr->ref = pointer_impl{.pointer_target = value};
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::store_as_pointer(local_index slot, pointer_impl value)
{
    auto& local_ptr = get_current_frame().local_values[slot];

    // TODO: is this correct?
    local_ptr = nullptr;

    create_local_value(slot, true);
    local_ptr->ref = value;
}

quxlang::type_symbol quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::procedure_symbol(type_symbol routine, std::string calling_convention) const
{
    for (char& c : calling_convention)
    {
        if (c >= 'A' && c <= 'Z')
        {
            c = static_cast< char >(c - 'A' + 'a');
        }
    }

    return submember{.of = std::move(routine), .name = "__procedure_" + calling_convention};
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_or_create_procedure(type_symbol routine, std::string calling_convention)
{
    auto routine_symbol = routine;
    auto symbol = procedure_symbol(std::move(routine), calling_convention);
    if (!functanoids3.contains(routine_symbol))
    {
        missing_functanoids_val.insert(std::move(routine_symbol));
    }

    auto& proc_local = m_procedures[symbol];
    if (proc_local == nullptr)
    {
        procedure_type proc_type;
        proc_type.calling_convention = std::move(calling_convention);
        proc_local = create_object(proc_type);
        proc_local->procedure = symbol;
        begin_lifetime(proc_local);
    }

    return proc_local;
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_or_create_antestatal_global(type_symbol symbol, std::optional< type_symbol > expected_type)
{
    bool const has_data = has_constexpr_antestatal_global(symbol);
    if (!has_data)
    {
        if (typeis< static_local_ref >(symbol) || typeis< static_snapshot_ref >(symbol))
        {
            throw compiler_bug("missing static localdata for " + quxlang::to_string(symbol));
        }
        mark_missing_antestatal_global(symbol);
    }

    if (!has_data && !expected_type.has_value())
    {
        throw constexpr_logic_execution_error("attempted to access non-antestatal global from antestatal value: " + quxlang::to_string(symbol));
    }

    auto data_it = constexpr_antestatal_global_values.find(symbol);
    auto type_it = constexpr_antestatal_global_types.find(symbol);
    if (type_it == constexpr_antestatal_global_types.end())
    {
        if (!expected_type.has_value())
        {
            throw constexpr_logic_execution_error("missing antestatal type for global: " + quxlang::to_string(symbol));
        }

        type_it = constexpr_antestatal_global_types.emplace(symbol, *expected_type).first;
    }
    else if (expected_type.has_value() && *expected_type != type_it->second)
    {
        throw constexpr_logic_execution_error("antestatal global reference type mismatch for: " + quxlang::to_string(symbol));
    }

    auto& root = antestatal_global_roots[symbol];
    if (root == nullptr)
    {
        auto const& global_type = type_it->second;
        root = create_object(global_type);
        root->antestatal_static_symbol = symbol;
        if (data_it != constexpr_antestatal_global_values.end())
        {
            initialize_local_from_antestatal_value(root, global_type, data_it->second);
            bool const is_mutable = constexpr_antestatal_global_mutable.contains(symbol) && constexpr_antestatal_global_mutable.at(symbol);
            if (!is_mutable)
            {
                set_readonly_tree(root);
            }
        }
    }

    return root;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::initialize_local_from_antestatal_value(std::shared_ptr< local > const& object, type_symbol type, antestatal_value const& value)
{
    if (!object)
    {
        throw compiler_bug("cannot initialize null antestatal object");
    }

    if (typeis< nvalue_slot >(type))
    {
        type = as< nvalue_slot >(type).target;
    }
    else if (typeis< dvalue_slot >(type))
    {
        type = as< dvalue_slot >(type).target;
    }

    if (typeis< storage >(type) || typeis< aligned_storage >(type))
    {
        if (typeis< antestatal_primitive >(value))
        {
            object->data = as< antestatal_primitive >(value).value;
            begin_lifetime(object);
            return;
        }
        throw constexpr_logic_execution_error("antestatal storage values are only supported as primitive bytes");
    }

    if (typeis< antestatal_interface >(value))
    {
        antestatal_interface const& interface_value = as< antestatal_interface >(value);
        if (type != interface_value.interface_type)
        {
            throw constexpr_logic_execution_error("antestatal interface initializer type mismatch");
        }
        object->interface_value = interface_object{
            .interface_type = interface_value.interface_type,
            .functions = interface_value.functions,
            .is_default = interface_value.is_default,
        };
        begin_lifetime(object);
        return;
    }

    if (typeis< ptrref_type >(type))
    {
        if (!typeis< antestatal_ptrref >(value))
        {
            throw constexpr_logic_execution_error("antestatal pointer/reference initializer has non-pointer value");
        }
        object->ref = pointer_from_antestatal_access(as< antestatal_ptrref >(value).target, as< ptrref_type >(type));
        begin_lifetime(object);
        return;
    }

    if (typeis< procedure_type >(type))
    {
        throw constexpr_logic_execution_error("antestatal procedure values are not representable yet");
    }

    if (typeis< array_type >(type))
    {
        if (!typeis< antestatal_array >(value))
        {
            throw constexpr_logic_execution_error("antestatal array initializer has non-array value");
        }
        auto const& array_ty = as< array_type >(type);
        auto const& array_value = as< antestatal_array >(value);
        if (object->array_members.size() != array_value.elements.size())
        {
            throw constexpr_logic_execution_error("antestatal array initializer length mismatch");
        }
        for (std::size_t i = 0; i < array_value.elements.size(); ++i)
        {
            initialize_local_from_antestatal_value(object->array_members.at(i), array_ty.element_type, array_value.elements.at(i));
        }
        begin_lifetime(object);
        return;
    }

    if (typeis< readonly_constant >(type))
    {
        if (!typeis< antestatal_struct >(value))
        {
            throw constexpr_logic_execution_error("antestatal readonly constant initializer has non-struct value");
        }
        auto byte_ptr_type = ptrref_type{.target = byte_type{}, .ptr_class = pointer_class::array, .qual = qualifier::constant};
        auto const& struct_value = as< antestatal_struct >(value);
        for (auto const& field_name : {"__start", "__end"})
        {
            auto value_it = struct_value.fields.find(field_name);
            auto member_it = object->struct_members.find(field_name);
            if (value_it != struct_value.fields.end() && member_it != object->struct_members.end())
            {
                initialize_local_from_antestatal_value(member_it->second, byte_ptr_type, value_it->second);
            }
        }
        begin_lifetime(object);
        return;
    }

    if (class_layouts.contains(type))
    {
        if (!typeis< antestatal_struct >(value))
        {
            throw constexpr_logic_execution_error("antestatal struct initializer has non-struct value");
        }
        auto const& layout = class_layouts.at(type);
        auto const& struct_value = as< antestatal_struct >(value);
        for (auto const& field : layout.fields)
        {
            type_symbol field_type = field.type;
            if (typeis< attached_type_reference >(field_type))
            {
                attached_type_reference const& attached = as< attached_type_reference >(field_type);
                if (typeis< void_type >(attached.carrying_type))
                {
                    continue;
                }
                field_type = attached.carrying_type;
            }
            auto member_it = object->struct_members.find(field.name);
            auto value_it = struct_value.fields.find(field.name);
            if (member_it == object->struct_members.end() || value_it == struct_value.fields.end())
            {
                throw constexpr_logic_execution_error("antestatal struct initializer missing field: " + field.name);
            }
            initialize_local_from_antestatal_value(member_it->second, field_type, value_it->second);
        }
        begin_lifetime(object);
        return;
    }

    if (!typeis< antestatal_primitive >(value))
    {
        throw constexpr_logic_execution_error("antestatal primitive initializer has non-primitive value");
    }
    object->data = as< antestatal_primitive >(value).value;
    begin_lifetime(object);
}

quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_impl quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::pointer_from_antestatal_access(antestatal_access const& access, ptrref_type const& ptr_type)
{
    if (typeis< antestatal_nullptr >(access))
    {
        return {};
    }

    if (typeis< antestatal_access_array_element >(access))
    {
        auto const& array_element = as< antestatal_access_array_element >(access);
        auto array = local_from_antestatal_access(array_element.array);
        if (array_element.index == array->array_members.size())
        {
            return pointer_impl{.one_past_the_end = array};
        }
        if (array_element.index > array->array_members.size())
        {
            throw constexpr_logic_execution_error("antestatal pointer array element is out of bounds");
        }
        return pointer_impl{.pointer_target = array->array_members.at(static_cast< std::size_t >(array_element.index))};
    }

    if (typeis< procedure_type >(ptr_type.target) && typeis< antestatal_access_global >(access))
    {
        auto const& procedure = as< antestatal_access_global >(access);
        auto const& expected_proc = as< procedure_type >(ptr_type.target);
        return pointer_impl{.pointer_target = get_or_create_procedure(procedure.symbol, expected_proc.calling_convention)};
    }

    return pointer_impl{.pointer_target = local_from_antestatal_access(access)};
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local_from_antestatal_access(antestatal_access const& access)
{
    if (typeis< antestatal_nullptr >(access))
    {
        throw constexpr_logic_execution_error("nullptr is not an antestatal object access");
    }

    if (typeis< antestatal_access_global >(access))
    {
        auto const& global = as< antestatal_access_global >(access);
        return get_or_create_antestatal_global(global.symbol);
    }

    if (typeis< antestatal_access_field >(access))
    {
        auto const& field = as< antestatal_access_field >(access);
        auto object = local_from_antestatal_access(field.object);
        auto member_it = object->struct_members.find(field.field_name);
        if (member_it == object->struct_members.end())
        {
            throw constexpr_logic_execution_error("antestatal field access cannot be resolved: " + field.field_name);
        }
        return member_it->second;
    }

    auto const& array_element = as< antestatal_access_array_element >(access);
    auto array = local_from_antestatal_access(array_element.array);
    if (array_element.index >= array->array_members.size())
    {
        throw constexpr_logic_execution_error("antestatal array access cannot be resolved to an object");
    }
    return array->array_members.at(static_cast< std::size_t >(array_element.index));
}

quxlang::antestatal_value quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::materialize_antestatal_value(std::shared_ptr< local > const& object, type_symbol type)
{
    if (!object || !object->alive())
    {
        throw constexpr_logic_execution_error("antestatal result materialization requires a live object");
    }

    if (typeis< nvalue_slot >(type))
    {
        type = as< nvalue_slot >(type).target;
    }
    else if (typeis< dvalue_slot >(type))
    {
        type = as< dvalue_slot >(type).target;
    }

    if (typeis< storage >(type) || typeis< aligned_storage >(type))
    {
        if (object->storage_active_type.has_value() && object->stored_object != nullptr)
        {
            return materialize_antestatal_value(object->stored_object, *object->storage_active_type);
        }
        return antestatal_primitive{.value = object->data};
    }

    if (object->interface_value.has_value())
    {
        interface_object const& interface_value = *object->interface_value;
        if (type != interface_value.interface_type)
        {
            throw constexpr_logic_execution_error("antestatal interface materialization type mismatch");
        }
        return antestatal_interface{
            .interface_type = interface_value.interface_type,
            .functions = interface_value.functions,
            .is_default = interface_value.is_default,
        };
    }

    if (typeis< ptrref_type >(type))
    {
        if (!object->ref.has_value())
        {
            throw constexpr_logic_execution_error("antestatal pointer materialization requires an initialized pointer/reference");
        }
        return antestatal_ptrref{.target = materialize_antestatal_access(*object->ref, as< ptrref_type >(type))};
    }

    if (typeis< procedure_type >(type))
    {
        throw constexpr_logic_execution_error("antestatal procedure values are not representable yet");
    }

    if (typeis< array_type >(type))
    {
        auto const& array_ty = as< array_type >(type);
        antestatal_array result;
        result.elements.reserve(object->array_members.size());
        for (auto const& element : object->array_members)
        {
            result.elements.push_back(materialize_antestatal_value(element, array_ty.element_type));
        }
        return result;
    }

    if (typeis< readonly_constant >(type))
    {
        auto byte_ptr_type = ptrref_type{.target = byte_type{}, .ptr_class = pointer_class::array, .qual = qualifier::constant};
        antestatal_struct result;
        for (auto const& field_name : {"__start", "__end"})
        {
            auto it = object->struct_members.find(field_name);
            if (it != object->struct_members.end())
            {
                result.fields[field_name] = materialize_antestatal_value(it->second, byte_ptr_type);
            }
        }
        return result;
    }

    if (class_layouts.contains(type))
    {
        antestatal_struct result;
        auto const& layout = class_layouts.at(type);
        for (auto const& field : layout.fields)
        {
            type_symbol field_type = field.type;
            if (typeis< attached_type_reference >(field_type))
            {
                attached_type_reference const& attached = as< attached_type_reference >(field_type);
                if (typeis< void_type >(attached.carrying_type))
                {
                    continue;
                }
                field_type = attached.carrying_type;
            }
            auto it = object->struct_members.find(field.name);
            if (it == object->struct_members.end())
            {
                throw constexpr_logic_execution_error("antestatal struct materialization missing field: " + field.name);
            }
            result.fields[field.name] = materialize_antestatal_value(it->second, field_type);
        }
        return result;
    }

    return antestatal_primitive{.value = object->data};
}

quxlang::antestatal_access quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::materialize_antestatal_access(pointer_impl ptr, ptrref_type const& ptr_type)
{
    if (pointer_invalidated(ptr))
    {
        throw constexpr_logic_execution_error("antestatal pointer materialization cannot represent an invalidated pointer");
    }

    if (pointer_is_nullptr(ptr))
    {
        return antestatal_nullptr{};
    }

    if (pointer_is_one_past_the_end(ptr))
    {
        auto array = ptr.one_past_the_end->lock();
        if (!array)
        {
            throw constexpr_logic_execution_error("antestatal pointer materialization cannot represent an expired one-past pointer");
        }
        auto array_access = access_to_antestatal_local(array);
        if (!array_access.has_value())
        {
            throw constexpr_logic_execution_error("antestatal pointer materialization cannot anchor one-past pointer");
        }
        return antestatal_access_array_element{.array = *array_access, .index = static_cast< std::uint64_t >(array->array_members.size())};
    }

    if (!ptr.pointer_target.has_value())
    {
        throw compiler_bug("antestatal pointer materialization saw non-null pointer without a target");
    }

    auto target = ptr.pointer_target->lock();
    if (!target)
    {
        throw constexpr_logic_execution_error("antestatal pointer materialization cannot represent an expired pointer");
    }

    if (typeis< procedure_type >(ptr_type.target))
    {
        if (!target->procedure.has_value())
        {
            throw constexpr_logic_execution_error("antestatal procedure pointer materialization requires a procedure target");
        }

        auto routine = *target->procedure;
        if (typeis< submember >(routine))
        {
            auto const& procedure_submember = as< submember >(routine);
            if (procedure_submember.name.starts_with("__procedure_"))
            {
                routine = procedure_submember.of;
            }
        }

        return antestatal_access_global{.symbol = routine};
    }

    auto access = access_to_antestatal_local(target);
    if (!access.has_value())
    {
        throw constexpr_logic_execution_error("antestatal pointer materialization cannot anchor pointer target");
    }

    return *access;
}

std::optional< quxlang::antestatal_access > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::access_to_antestatal_local(std::shared_ptr< local > const& target)
{
    if (!target)
    {
        return std::nullopt;
    }

    if (auto root = constexpr_result_root.lock(); root && constexpr_result_global_symbol.has_value())
    {
        if (auto access = access_to_antestatal_subobject(root, target, antestatal_access_global{.symbol = *constexpr_result_global_symbol}))
        {
            return access;
        }
    }

    auto global_root = target;
    while (global_root)
    {
        if (global_root->antestatal_static_symbol.has_value())
        {
            if (auto access = access_to_antestatal_subobject(global_root, target, antestatal_access_global{.symbol = *global_root->antestatal_static_symbol}))
            {
                return access;
            }
            return std::nullopt;
        }

        if (global_root->member_of.has_value())
        {
            global_root = global_root->member_of->lock();
        }
        else
        {
            break;
        }
    }

    return std::nullopt;
}

std::optional< quxlang::antestatal_access > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::access_to_antestatal_subobject(std::shared_ptr< local > const& object, std::shared_ptr< local > const& target, antestatal_access access)
{
    if (!object)
    {
        return std::nullopt;
    }
    if (object == target)
    {
        return access;
    }

    for (auto const& [field_name, field] : object->struct_members)
    {
        auto field_access = antestatal_access_field{.object = access, .field_name = field_name};
        if (auto result = access_to_antestatal_subobject(field, target, field_access))
        {
            return result;
        }
    }

    for (std::size_t i = 0; i < object->array_members.size(); ++i)
    {
        auto element_access = antestatal_access_array_element{.array = access, .index = static_cast< std::uint64_t >(i)};
        if (auto result = access_to_antestatal_subobject(object->array_members.at(i), target, element_access))
        {
            return result;
        }
    }

    if (object->stored_object)
    {
        return access_to_antestatal_subobject(object->stored_object, target, access);
    }

    return std::nullopt;
}

bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::is_reference_type(quxlang::vmir2::local_index slot)
{
    throw compiler_bug("removed");
}
bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::is_pointer_type(quxlang::vmir2::local_index slot)
{
    throw compiler_bug("removed");
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::transition(quxlang::vmir2::block_index block)
{

    transition3(block);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::transition3(quxlang::vmir2::block_index block)
{
    std::vector< vmir2::local_index > values_to_destroy;

    auto original_block = get_current_frame().address.block;

    auto& current_frame = get_current_frame();
    auto const& current_func_ir = current_frame.ir3;

    auto const& target_block = current_func_ir->blocks.at(block);

    std::set< vmir2::local_index > current_values;
    std::set< vmir2::local_index > entry_values;

    for (auto& [idx, local] : current_frame.local_values)
    {
        if (local != nullptr)
        {
            if (local->alive() && !target_block.entry_state.contains(idx))
            {
                auto slot_type = current_func_ir->local_types.at(idx).type;
                abort_initguard_lock_if_needed(slot_type, local);
                bool local_is_delegate_alias = local->member_of.has_value() || local->storage_owner.has_value() || local->initializer_of.has_value() || local->array_init_member_of.has_value();
                bool local_has_nontrivial_dtor = current_func_ir->non_trivial_dtors.contains(slot_type);
                if (local_has_nontrivial_dtor && !local_is_delegate_alias)
                {
                    auto dtor = current_func_ir->non_trivial_dtors.at(slot_type);
                    call_func(dtor, {.named = {{"THIS", idx}}});
                    // We return because we don't want to double stack dtor frames, we are only looking for singular violations.
                    return;
                }
                else
                {
                    local = nullptr;
                }
            }
            else if (!target_block.entry_state.contains(idx))
            {
                // If the local is not alive, we can safely remove it
                local = nullptr;
            }
        }

        if (target_block.entry_state.contains(idx) && target_block.entry_state.at(idx).alive() && (local == nullptr || local->alive() == false))
        {
            auto idxvar = idx;

            std::string error_msg = "Error in [transition]: slot " + std::to_string(idxvar) + " is not alive, but should be";
            throw compiler_bug(error_msg);
        }
    }

    current_frame.address.block = block;
    current_frame.address.instruction_index = 0;
}

bool quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::transition_normal_exit()
{
    // TODO: This has a lot of duplicated code with `transition3`, consider refactoring.

    // TODO: This function doesn't take int account all possible exit transitions, namely DVALUE slots are not handled correctly.
    std::vector< vmir2::local_index > values_to_destroy;

    auto original_block = get_current_frame().address.block;

    auto& current_frame = get_current_frame();
    auto const& current_func_ir = current_frame.ir3;

    state_map exit_state;
    codegen_state_engine(exit_state, current_func_ir->local_types, current_func_ir->parameters).apply_normal_exit();

    std::set< vmir2::local_index > current_values;
    std::set< vmir2::local_index > entry_values;

    auto value_should_be_alive = [&](vmir2::local_index idx) -> bool
    {
        if (exit_state.contains(idx))
        {
            return exit_state.at(idx).alive();
        }
        return false;
    };

    std::set< vmir2::local_index > destroy_values;
    std::set< vmir2::local_index > new_values;

    auto handle_routine_parameter = [&](routine_parameter const& param)
    {
        if (param.type.type_is< dvalue_slot >())
        {
            destroy_values.insert(param.local_index);
        }

        if (param.type.type_is< nvalue_slot >())
        {
            new_values.insert(param.local_index);
        }
    };
    // neeed to loop over parameters and check which ones are DESTROY[T] slots.
    for (auto const& [_, param] : current_func_ir->parameters.named)
    {
        handle_routine_parameter(param);
    }

    for (auto const& param : current_func_ir->parameters.positional)
    {
        handle_routine_parameter(param);
    }

    for (auto& [idx, local] : current_frame.local_values)
    {
        auto lidx = idx;
        if (local != nullptr)
        {
            if (destroy_values.contains(idx))
            {
                // If the local is a DESTROY[T] slot, then returning from this function
                // sets it to not alive by definition.
                // However, this does not eliminate the underlying storage.
                local->stage = slot_stage::dead;
                local->delegates = std::nullopt;
                if (local->storage_owner.has_value())
                {
                    auto owner = local->storage_owner.value().lock();
                    if (owner != nullptr)
                    {
                        owner->storage_active_type = std::nullopt;
                        owner->stored_object = nullptr;
                    }
                    local->storage_owner = std::nullopt;
                }
                local->storage_destroy_delegate = false;
            }
            else if (new_values.contains(idx))
            {
                // If we have a NEW[T] slot, then returning from this function
                // transitions from partial to full by definition.
                assert(local->stage != slot_stage::dead);
                // Note: array initializers will already construct the object, that's okay.
                begin_lifetime(local);
                if (local->storage_owner.has_value())
                {
                    auto owner = local->storage_owner.value().lock();
                    if (owner != nullptr)
                    {
                        owner->storage_active_type = local->storage_projection_type;
                        owner->stored_object = local;
                    }
                }
                local->storage_destroy_delegate = false;
            }
            else if (local->alive() && !value_should_be_alive(idx))
            {
                auto slot_type = current_func_ir->local_types.at(idx).type;
                abort_initguard_lock_if_needed(slot_type, local);
                if ((typeis< storage >(slot_type) || typeis< aligned_storage >(slot_type)) && local->storage_active_type.has_value())
                {
                    throw constexpr_logic_execution_error("storage lifetime ended while containing an active object");
                }
                bool local_is_delegate_alias = local->member_of.has_value() || local->storage_owner.has_value() || local->initializer_of.has_value() || local->array_init_member_of.has_value();
                bool local_has_nontrivial_dtor = current_func_ir->non_trivial_dtors.contains(slot_type);
                if (local_has_nontrivial_dtor && !local_is_delegate_alias)
                {
                    auto dtor = current_func_ir->non_trivial_dtors.at(slot_type);
                    call_func(dtor, {.named = {{"THIS", idx}}});
                    // We return because we don't want to double stack dtor frames, we are only looking for singular violations.
                    return false;
                }
                else
                {
                    local = nullptr;
                }
            }
            else if (!value_should_be_alive(idx))
            {
                // If the local is not alive, we can safely remove it
                local = nullptr;
            }
        }

        if (value_should_be_alive(idx) && (local == nullptr || local->alive() == false))
        {
            auto idxvar = idx;
            throw compiler_bug("Error in [transition]: slot not alive");
        }
    }

    return true;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::increment const& instr)
{
    exec_instr_val_incdec(instr.value, instr.result, true, true);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::begin_lifetime(std::shared_ptr< local > object)
{

    assert(object != nullptr);

    if (object->member_of.has_value())
    {
        assert(object->storage_initiated);
    }
    object->storage_initiated = true;

    for (auto& [name, member] : object->struct_members)
    {
        if (member == nullptr)
        {
            throw compiler_bug("struct member not initialized");
        }
        assert(member->alive());
    }

    for (std::size_t i = 0; i < object->array_members.size(); i++)
    {
        auto& member = object->array_members.at(i);
        if (member == nullptr)
        {
            throw compiler_bug("array member not initialized");
        }
        assert(member->alive());
    }

    object->stage = slot_stage::full;

    if (object->storage_owner.has_value())
    {
        auto owner = object->storage_owner.value().lock();
        if (owner != nullptr)
        {
            owner->storage_active_type = object->storage_projection_type;
            owner->stored_object = object;
            auto storage_symbol = global_storage_symbols.find(owner.get());
            if (storage_symbol != global_storage_symbols.end() && has_constexpr_antestatal_global(storage_symbol->second))
            {
                object->antestatal_static_symbol = storage_symbol->second;
            }
        }
    }

    if (object->array_init_member_of.has_value())
    {
        auto array_init = object->array_init_member_of.value().lock();

        array_init->init_count++;
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::end_lifetime(std::shared_ptr< local > object)
{
    object->stage = slot_stage::dead;
    if (object->storage_owner.has_value())
    {
        auto owner = object->storage_owner.value().lock();
        if (owner != nullptr)
        {
            owner->storage_active_type = std::nullopt;
            owner->stored_object = nullptr;
        }
    }
    object->antestatal_static_symbol = std::nullopt;
    if (!object->member_of.has_value() && !object->storage_owner.has_value())
    {
        object->storage_initiated = false;
    }
}
std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::output_local(local_index at)
{
    auto& frame = get_current_frame();

    auto& slot_ptr = frame.local_values[at];
    if (slot_ptr == nullptr)
    {
        auto type = frame.ir3->local_types.at(at).type;
        slot_ptr = create_object(type);
    }
    assert(slot_ptr->stage == slot_stage::dead);

    begin_lifetime(slot_ptr);

    return slot_ptr;
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_or_create_global_storage(type_symbol symbol, type_symbol storage_type)
{
    if (!typeis< storage >(storage_type))
    {
        throw compiler_bug("global storage must be created with STORAGE(T)");
    }

    auto& storage_local = global_storages[symbol];
    if (storage_local == nullptr)
    {
        storage_local = create_object(storage_type);
        begin_lifetime(storage_local);
    }
    global_storage_symbols[storage_local.get()] = symbol;
    if (has_constexpr_antestatal_global(symbol))
    {
        ensure_antestatal_global_object(symbol, storage_local, storage_type);
    }

    return storage_local;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::ensure_antestatal_global_object(type_symbol symbol, std::shared_ptr< local > const& storage_local, type_symbol storage_type)
{
    if (!storage_local)
    {
        throw compiler_bug("antestatal global storage root missing");
    }
    if (!typeis< storage >(storage_type))
    {
        throw compiler_bug("antestatal global storage must be STORAGE(T)");
    }

    auto const& storable_types = as< storage >(storage_type).storable_types;
    if (storable_types.size() != 1)
    {
        throw compiler_bug("global storage should contain exactly one storable type");
    }

    auto object_type = *storable_types.begin();
    auto object = get_or_create_antestatal_global(symbol, object_type);
    storage_local->storage_active_type = object_type;
    storage_local->stored_object = object;
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_or_create_initguard(type_symbol symbol)
{
    auto& guard = global_initguards[symbol];
    if (guard == nullptr)
    {
        guard = create_object(initguard_type{});
        set_initguard_state(guard, initguard_state::uninitialized);
    }

    return guard;
}

quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::initguard_state quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::get_initguard_state(std::shared_ptr< local > const& guard)
{
    if (!guard)
    {
        throw compiler_bug("null initguard");
    }

    if (guard->data.empty())
    {
        return initguard_state::uninitialized;
    }

    switch (static_cast< std::uint8_t >(guard->data[0]))
    {
    case 0:
        return initguard_state::uninitialized;
    case 1:
        return initguard_state::initializing;
    case 2:
        return initguard_state::initialized;
    default:
        throw constexpr_logic_execution_error("invalid initguard state byte");
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::set_initguard_state(std::shared_ptr< local > const& guard, initguard_state state)
{
    if (!guard)
    {
        throw compiler_bug("null initguard");
    }

    guard->storage_initiated = true;
    guard->stage = slot_stage::full;
    guard->data.assign(1, std::byte{0});
    guard->data[0] = std::byte(static_cast< std::uint8_t >(state));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::set_initguard_lock(std::shared_ptr< local > const& lock, std::shared_ptr< local > const& guard)
{
    if (!lock || !guard)
    {
        throw compiler_bug("invalid initguard lock setup");
    }

    lock->storage_initiated = true;
    lock->stage = slot_stage::full;
    lock->data.assign(1, std::byte{1});
    lock->ref = pointer_impl{.pointer_target = guard};
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::abort_initguard_lock_if_needed(type_symbol slot_type, std::shared_ptr< local > const& lock)
{
    if (!typeis< initguard_lock_type >(slot_type) || !lock || !lock->alive() || !lock->ref.has_value() || !lock->ref->pointer_target.has_value())
    {
        return;
    }

    auto guard = lock->ref->pointer_target->lock();
    lock->ref = std::nullopt;
    if (!guard)
    {
        return;
    }

    if (get_initguard_state(guard) == initguard_state::initializing)
    {
        set_initguard_state(guard, initguard_state::uninitialized);
    }
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::constdata(std::vector< std::byte > const& data)
{
    auto& cell = this->global_constdata[data];
    if (cell == nullptr)
    {
        array_type constdata_type;
        constdata_type.element_type = byte_type{};
        constdata_type.element_count = expression_numeric_literal{.value = std::to_string(data.size())};
        cell = create_object(constdata_type);
        assert(cell->array_members.size() == data.size());
        for (std::size_t i = 0; i < data.size(); ++i)
        {
            auto cell_member = cell->array_members.at(i);
            local_set_data(cell_member, {data[i]});
            cell_member->readonly = true;
        }
        cell->readonly = true;
        begin_lifetime(cell);
    }

    return cell;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val_incdec(local_index val, local_index result, bool increment, bool postfix)
{
    auto const& type = frame_slot_data_type(val);

    if (!typeis< ptrref_type >(type))
    {
        throw invalid_instruction_error("Expected reference to int or pointer to increment, but got " + to_string(type) + " instead");
    }

    auto type_int_or_pointer_g = type.get_as< ptrref_type >().target;

    if (typeis< int_type >(type_int_or_pointer_g))
    {
        exec_incdec_int(val, result, increment, postfix);
    }
    else if (typeis< ptrref_type >(type_int_or_pointer_g))
    {
        exec_incdec_ptr(val, result, increment, postfix);
    }
    else
    {
        throw invalid_instruction_error("Error in [increment]: slot is not an int or pointer");
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_incdec_int(local_index input_slot, local_index output_slot, bool increment, bool postfix)
{
    auto const& type = frame_slot_data_type(input_slot);
    auto type_int_g = type.get_as< ptrref_type >().target;

    int_type const& type_int = type_int_g.get_as< int_type >();

    auto value_to_increment = load_from_reference(input_slot, true);

    auto val = local_consume_data(value_to_increment);
    auto original_val = val;

    if (increment)
    {
        val = bytemath::detail::unlimited_int_unsigned_add_le_raw(std::move(val), {std::byte(1)});
    }
    else
    {
        val = bytemath::detail::unlimited_int_unsigned_sub_le_raw(std::move(val), {std::byte(1)});
    }
    val = bytemath::detail::le_truncate_raw(std::move(val), type_int.bits);

    local_set_data(value_to_increment, std::move(val));

    if (postfix)
    {
        auto result_local = output_local(output_slot);
        local_set_data(result_local, std::move(original_val));
    }
    else
    {
        store_as_reference(output_slot, value_to_increment);
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_incdec_ptr(local_index input_slot, local_index output_slot, bool increment, bool postfix)
{
    auto value_to_increase = load_from_reference(input_slot, true);

    auto& ptr = value_to_increase->ref;

    auto const& pointer_ref_type_v = get_local_type(input_slot);

    if (!typeis< ptrref_type >(pointer_ref_type_v) || !as< ptrref_type >(pointer_ref_type_v).target.type_is< ptrref_type >())
    {
        // TODO: Also double check this is array pointer
        throw invalid_instruction_error("Error in [increment]: slot is not a reference");
    }

    auto const& pointer_type_v = as< ptrref_type >(pointer_ref_type_v).target.as< ptrref_type >();

    if (!ptr.has_value())
    {
        ptr = pointer_impl{};
    }

    auto original_ptr = ptr;

    // TODO: include type?
    ptr = pointer_arith(ptr.value(), increment ? 1 : -1, void_type{});

    if (postfix)
    {
        auto result_local = output_local(output_slot);
        result_local->ref = original_ptr;
    }
    else
    {
        this->store_as_reference(output_slot, value_to_increase);
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::assert_instr const& asrt)
{
    auto reg = asrt.condition;

    auto data = consume_local_as_data(reg);

    if (data == std::vector< std::byte >{std::byte{0}})
    {
        throw constexpr_logic_execution_error("assertion failed: " + asrt.message);
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::decrement const& instr)
{
    exec_instr_val_incdec(instr.value, instr.result, false, true);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::preincrement const& instr)
{
    exec_instr_val_incdec(instr.target, instr.target2, true, false);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::predecrement const& instr)
{
    exec_instr_val_incdec(instr.target, instr.target2, false, false);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::pointer_arith const& par)
{

    auto ptr = load_as_pointer(par.from, true);

    // quxlang::bytemath::le_sint multiplier;

    auto const& offset_type = get_local_type(par.offset);

    if (!offset_type.type_is< int_type >())
    {
        throw compiler_bug("Error in [pointer_arith]: offset is not an integer");
    }

    auto offset_val = consume_local_as_data(par.offset);

    std::int64_t offset = 0;
    bool ok = false;
    auto offset_unlimited = bytemath::le_int_fixed_to_unlimited(get_fixed_int_options(offset_type), offset_val);

    if (par.multiplier == -1)
    {
        auto offset_val_s = bytemath::unlimited_int_signed_sub_le(bytemath::sle_int_unlimited{0}, std::move(offset_unlimited));
        std::tie(offset, ok) = offset_val_s.to_int< std::int64_t >();
    }
    else
    {
        std::tie(offset, ok) = offset_unlimited.to_int< std::int64_t >();
    }

    if (!ok)
    {
        throw constexpr_logic_execution_error("pointer_arith offset overflow in constexpr execution");
    }

    type_symbol const& ptrref_type = get_local_type(par.result);
    pointer_impl new_ptr = pointer_arith(ptr, offset, ptrref_type);

    store_as_pointer(par.result, new_ptr);
}
std::vector< std::byte > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::use_data(local_index slot)
{
    auto slot_ptr = get_current_frame().local_values[slot];

    if (slot_ptr == nullptr)
    {
        throw compiler_bug("Error in [use_data]: slot not allocated");
    }

    if (!slot_ptr->alive())
    {
        throw compiler_bug("Error in [use_data]: slot not alive");
    }

    if (slot_ptr->ref.has_value())
    {
        auto& ref = *slot_ptr->ref->pointer_target.value().lock();
        return ref.data;
    }
    else
    {
        return slot_ptr->data;
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::pointer_diff const& pdf)
{
    auto from = load_as_pointer(pdf.from, true);
    auto to = load_as_pointer(pdf.to, true);

    auto from_array = pointer_memberof(from);
    auto to_array = pointer_memberof(to);
    if (!from_array.has_value() || !to_array.has_value() || from_array->lock() != to_array->lock())
    {
        throw constexpr_logic_execution_error("pointer difference requires pointers into the same array");
    }

    auto array = from_array->lock();
    auto diff = pointer_offset_in_array(array, from) - pointer_offset_in_array(array, to);
    auto result_type = get_local_type(pdf.result);
    auto byte_result = bytemath::unlimited_to_fixed(get_fixed_int_options(result_type), bytemath::sle_int_unlimited{diff});
    if (byte_result.result_is_undefined)
    {
        throw constexpr_logic_execution_error("overflow in pointer difference");
    }

    auto result = output_local(pdf.result);
    local_set_data(result, std::move(byte_result.data_bytes));
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::array_init_start const& ais)
{
    auto& frame = get_current_frame();

    auto& array = frame.local_values[ais.on_value];
    if (array == nullptr)
    {
        array = create_object(get_local_type(ais.on_value));
    }

    auto initializer = output_local(ais.initializer);

    auto initializer_type = get_local_type(ais.initializer);

    if (!initializer_type.template type_is< array_initializer_type >()) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_start expects __ARRAY_INITAILIZER");
    }

    initializer->initializer_of = array;
    auto delgs = invocation_args{};
    array->stage = slot_stage::partial;
    delgs.named["INIT"] = ais.initializer;
    array->delegates = delgs;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::array_init_element const& aie)
{
    auto initializer = get_current_frame().local_values[aie.initializer];

    if (initializer == nullptr) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_element: initializer not found");
    }

    auto array = initializer->initializer_of.value().lock();
    if (array == nullptr || !array->alive()) [[unlikely]]
    {
        throw invalid_instruction_transition_error("initializer refers to out of scope array");
    }

    std::size_t element_id = initializer->init_count;

    if (element_id >= array->array_members.size()) [[unlikely]]
    {
        throw constexpr_logic_execution_error("initializing element out of bounds of array");
    }

    auto element_cell = array->array_members.at(element_id);
    assert(element_cell->stage == slot_stage::dead);

    element_cell->array_init_member_of = initializer;
    current_local(aie.target) = element_cell;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::array_init_index const& air)
{
    auto initializer = current_local(air.initializer);

    if (initializer == nullptr) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_index: initializer not found");
    }

    auto initializer_type = frame_slot_data_type(air.initializer);

    if (!initializer_type.template type_is< array_initializer_type >()) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_index expects __ARRAY_INITAILIZER");
    }

    auto const& initializer_type_t = initializer_type.get_as< array_initializer_type >();

    auto count = initializer_type_t.count;

    auto idx = initializer->init_count;

    auto& result = current_local(air.result);

    auto result_type = frame_slot_data_type(air.result);

    if (!result_type.template type_is< int_type >())
    {
        throw invalid_instruction_transition_error("array_init_index expects int result type");
    }
    auto const& int_type_v = result_type.get_as< int_type >();

    auto idx_bytes = bytemath::u_to_le< std::uint64_t >(idx);

    bytemath::fixed_int_options opts{};

    opts.has_sign = int_type_v.has_sign;
    opts.bits = int_type_v.bits;
    opts.overflow_undefined = true;

    auto byte_result = bytemath::unlimited_to_fixed(opts, idx_bytes);

    if (byte_result.result_is_undefined)
    {
        throw constexpr_logic_execution_error("overflow in array_init_index");
    }
    result = create_local_value(air.result, false);
    local_set_data(result, std::move(byte_result.data_bytes));
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::array_init_more const& aim)
{
    auto initializer = current_local(aim.initializer);

    if (initializer == nullptr) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_index: initializer not found");
    }

    auto initializer_type = frame_slot_data_type(aim.initializer);

    if (!initializer_type.template type_is< array_initializer_type >()) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_index expects __ARRAY_INITAILIZER");
    }

    auto const& initializer_type_t = initializer_type.get_as< array_initializer_type >();

    auto count = initializer_type_t.count;

    auto idx = initializer->init_count;

    auto& result = current_local(aim.result);

    auto result_type = frame_slot_data_type(aim.result);

    if (!result_type.template type_is< bool_type >()) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_index expects bool result type");
    }

    result = create_local_value(aim.result, false);

    assert(idx <= count);
    if (idx != count)
    {
        local_set_data(result, {std::byte{1}});
    }
    else
    {
        local_set_data(result, {std::byte{0}});
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::array_init_finish const& aif)
{
    auto& initializer = current_local(aif.initializer);

    if (initializer == nullptr) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_finish: initializer not found");
    }

    auto initializer_type = frame_slot_data_type(aif.initializer);

    if (!initializer_type.template type_is< array_initializer_type >()) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_finish expects __ARRAY_INITAILIZER");
    }

    auto const& initializer_type_t = initializer_type.get_as< array_initializer_type >();

    auto array_length = initializer_type_t.count;

    auto remaining = array_length - initializer->init_count;
    if (remaining != 0)
    {
        throw constexpr_logic_execution_error("array_init_finish: not all elements initialized");
    }

    auto array = initializer->initializer_of.value().lock();

    array->delegates = {};

    auto is_array_init_member_alias = [&](std::shared_ptr< local > const& local_value) -> bool
    {
        if (!local_value)
        {
            return false;
        }
        if (local_value->array_init_member_of.has_value() && local_value->array_init_member_of.value().lock() == initializer)
        {
            return true;
        }
        return local_value->member_of.has_value() && local_value->member_of.value().lock() == array;
    };
    for (auto& [idx, local] : get_current_frame().local_values)
    {
        if (is_array_init_member_alias(local))
        {
            local = nullptr;
        }
    }

    for (auto& member : array->array_members)
    {
        if (member && member->array_init_member_of.has_value() && member->array_init_member_of.value().lock() == initializer)
        {
            member->array_init_member_of.reset();
        }
    }
    begin_lifetime(array);
}
