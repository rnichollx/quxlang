// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#include <utility>

#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

#include "quxlang/backends/asm/arm_asm_converter.hpp"
#include "quxlang/bytemath.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/fixed_bytemath.hpp"
#include "quxlang/parsers/parse_int.hpp"
#include "quxlang/vmir2/assembler.hpp"
#include "rpnx/value.hpp"

#include <deque>

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
    std::size_t exec_mode = 1;
    std::map< cow< type_symbol >, class_layout > class_layouts;
    std::map< cow< type_symbol >, cow< functanoid_routine3 > > functanoids3;
    std::vector< std::byte > constexpr_result_v;
    std::optional< type_symbol > constexpr_result_type;
    std::optional< type_symbol > constexpr_result_type_value;

    std::set< type_symbol > missing_functanoids_val;

    std::uint64_t next_object_id = 1;

    struct local;
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

    struct local
    {
        std::vector< std::byte > data;
        bool negative;
        slot_stage stage = slot_stage::dead;
        bool storage_initiated = false;
        std::uint64_t object_id{};
        bool readonly = false;
        std::optional< pointer_impl > ref;
        std::optional< std::weak_ptr< local > > member_of;
        std::optional< std::weak_ptr< local > > initializer_of;
        std::optional< std::weak_ptr< local > > array_init_member_of;
        std::optional< dtor_spec > dtor;
        std::vector< std::shared_ptr< local > > array_members;
        std::map< std::string, std::shared_ptr< local > > struct_members;
        std::optional< invocation_args > delegates;
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

    void call_func(cow< type_symbol > functype, vmir2::invocation_args args);
    void exec();
    void exec3();

    quxlang::vmir2::state_diff get_state_diff();
    void exec_instr();
    void exec_instr3();

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

    std::size_t get_type_size(const type_symbol& type);

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

    void exec_instr_val_incdec(local_index val, local_index result, bool increment);

    void exec_incdec_int(local_index input_slot, local_index output_slot, bool increment);
    void exec_incdec_ptr(local_index input_slot, local_index output_slot, bool increment);

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
    void exec_instr_val(vmir2::runtime_ce const& rce);
    void exec_instr_val(vmir2::access_array const& aca);
    void exec_instr_val(vmir2::invoke const& inv);
    void exec_instr_val(vmir2::make_reference const& mrf);
    void exec_instr_val(vmir2::jump const& jmp);
    void exec_instr_val(vmir2::branch const& brn);
    void exec_instr_val(vmir2::cast_reference const& cst);
    void exec_instr_val(vmir2::constexpr_set_result const& csr);
    void exec_instr_val(vmir2::load_const_value const& lcv);
    void exec_instr_val(vmir2::make_pointer_to const& mpt);
    void exec_instr_val(vmir2::dereference_pointer const& drp);
    void exec_instr_val(vmir2::load_from_ref const& lfr);
    void exec_instr_val(vmir2::ret const& ret);
    void exec_instr_val(vmir2::int_add const& add);
    void exec_instr_val(vmir2::int_sub const& sub);
    void exec_instr_val(vmir2::int_mul const& mul);
    void exec_instr_val(vmir2::int_div const& div);
    void exec_instr_val(vmir2::int_mod const& mod);
    void exec_instr_val(vmir2::store_to_ref const& str);
    void exec_instr_val(vmir2::load_const_int const& lci);
    void exec_instr_val(vmir2::cmp_eq const& ceq);
    void exec_instr_val(vmir2::cmp_ne const& cne);
    void exec_instr_val(vmir2::cmp_lt const& clt);
    void exec_instr_val(vmir2::cmp_ge const& cge);
    void exec_instr_val(vmir2::pcmp_eq const& ceq);
    void exec_instr_val(vmir2::pcmp_ne const& cne);
    void exec_instr_val(vmir2::pcmp_lt const& clt);
    void exec_instr_val(vmir2::pcmp_ge const& cge);
    void exec_instr_val(vmir2::gcmp_eq const& ceq);
    void exec_instr_val(vmir2::gcmp_ne const& cne);
    void exec_instr_val(vmir2::gcmp_lt const& clt);
    void exec_instr_val(vmir2::gcmp_ge const& cge);
    void exec_instr_val(vmir2::defer_nontrivial_dtor const& dntd);
    void exec_instr_val(vmir2::struct_init_start const& sdn);
    void exec_instr_val(vmir2::struct_init_finish const& scn);
    void exec_instr_val(vmir2::copy_reference const& cpr);
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

    std::shared_ptr< local > load_from_reference(local_index local_idx, bool consume);
    pointer_impl load_as_pointer(local_index slot, bool consume);
    void store_as_reference(local_index slot, std::shared_ptr< local > value);
    void store_as_pointer(local_index slot, pointer_impl value);

    bool is_reference_type(quxlang::vmir2::local_index slot);
    bool is_pointer_type(quxlang::vmir2::local_index slot);

    std::vector< std::byte > use_data(local_index slot);

    std::vector< std::byte > slot_consume_data(vmir2::local_index slot);
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

    std::uint64_t consume_u64(local_index slot);
    std::shared_ptr< local > consume_local(local_index slot);

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

    for (auto const& func : functanoids3)
    {
        quxlang::vmir2::assembler ir_printer(func.second.get());

        std::cout << "Functanoid: " << quxlang::to_string(*(func.first)) << std::endl;

        std::cout << ir_printer.to_string(func.second.get()) << std::endl;
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

    auto current_func_ir = functanoids3.at(current_func.get());

    auto const& current_block = current_func_ir->blocks.at(current_instr_address.block);

    quxlang::vmir2::assembler ir_printer(current_func_ir.get());

    if (current_instr_address.instruction_index < current_block.instructions.size())
    {
        vm_instruction const& instr = current_block.instructions.at(current_instr_address.instruction_index);

        std::cout << "Executing in constexpr " << quxlang::to_string(current_func.get()) << " block " << current_instr_address.block << " instruction " << current_instr_address.instruction_index << ": " << ir_printer.to_string(instr) << std::endl;
        //  If there is an error here, it usually means there is an instruction which is not implemented
        //  on the constexpr virtual machine. Instructions which are illegal in a constexpr context
        //  should be implemented to throw a derivative of std::logic_error.

        auto expected_state = get_expected_state_map_preexec3(current_frame_index(), current_instr_address.block, current_instr_address.instruction_index);
        auto start_frame_id = current_frame_index();
        auto current_statemap = get_current_state_map3(current_frame_index());

        auto state_diff = this->get_state_diff();

        std::cout << " - Expected before state: " << ir_printer.to_string(expected_state) << std::endl;
        std::cout << " - Actual before state: " << ir_printer.to_string(current_statemap) << std::endl;

        if (current_statemap != expected_state)
        {
            for (auto const& [index, states] : state_diff)
            {
                std::cout << "   - Slot " << index << " state mismatch: expected " << ir_printer.to_string(index, states.second) << ", got " << ir_printer.to_string(index, states.first) << std::endl;
            }
        }
        assert(current_statemap == expected_state);

        std::size_t stack_size1 = stack.size();
        rpnx::apply_visitor< void >(
            [this](auto const& param)
            {
                return this->exec_instr_val(param);
            },
            instr);
        std::size_t stack_size2 = stack.size();
        current_instr_address.instruction_index++;
        std::cout << std::endl;
        return;
    }

    auto const terminator_instruction = current_block.terminator;
    if (!terminator_instruction)
    {
        throw constexpr_logic_execution_error("Constexpr execution reached end of block with undefined behavior");
    }

    std::cout << "Executing in constexpr " << quxlang::to_string(current_func.get()) << " block " << current_instr_address.block << " terminator " << current_instr_address.instruction_index << ": " << ir_printer.to_string(terminator_instruction.value()) << std::endl;

    rpnx::apply_visitor< void >(
        [this](auto const& param)
        {
            return this->exec_instr_val(param);
        },
        *terminator_instruction);
    return;
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

    std::string type_str = quxlang::to_string(type);

    if (typeis< int_type >(type))
    {
        return (type.get_as< int_type >().bits + 7) / 8;
    }

    if (typeis< byte_type >(type))
    {
        return 1;
    }

    if (typeis< bool_type >(type))
    {
        return 1;
    }

    // Pointer data is stored in ref, not data bytes.
    if (typeis< ptrref_type >(type))
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

    field->ref = pointer_impl{.pointer_target = field_slot};
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::swap const& swp)
{
    auto local_a = load_from_reference(swp.a, true);
    auto local_b = load_from_reference(swp.b, true);

    auto& frame = get_current_frame();

    std::swap(local_a->data, local_b->data);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::runtime_ce const& rce)
{
    // runtime_ce does not consume inputs; it simply produces true in the target bool slot
    auto const& type = get_local_type(rce.target);
    auto sz = get_type_size(type);

    if (type != bool_type{})
    {
        throw compiler_bug("RT_CE must write to a bool slot");
    }

    if (get_current_frame().local_values[rce.target] && get_current_frame().local_values[rce.target]->alive())
    {
        throw compiler_bug("Local value already has pointer");
    }

    auto local_ptr = output_local(rce.target);
    local_set_data(local_ptr, std::vector< std::byte >(sz, std::byte{1}));
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

        output_bool(tb.to, local->ref.has_value());
    }
    else
    {
        auto data = slot_consume_data(tb.from);

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

        output_bool(tbn.to, !local->ref.has_value());
    }
    else
    {
        auto data = slot_consume_data(tbn.from);

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

    auto& field_slot = ref_to_ptr->array_members.at(arry_index);

    field->ref = pointer_impl{.pointer_target = field_slot};
    begin_lifetime(field);

    parent_ref_slot = nullptr;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::invoke const& inv)
{
    call_func(inv.what, inv.args);
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

    auto data = slot_consume_data(reg);

    if (data == std::vector< std::byte >{std::byte{0}})
    {
        transition(brn.target_false);
    }
    else
    {
        transition(brn.target_true);
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::cast_reference const& cst)
{
    auto& local_ptr_base = get_current_frame().local_values.at(cst.source_ref_index);
    auto local_ptr_result = output_local(cst.target_ref_index);

    if (!local_ptr_base)
    {
        throw constexpr_logic_execution_error("Error executing <cast_reference>: accessing deallocated storage");
    }

    if (!local_ptr_base->alive())
    {
        throw constexpr_logic_execution_error("Error executing <cast_reference>: accessing dealived storage");
    }




    local_ptr_result->ref = local_ptr_base->ref;

    end_lifetime(local_ptr_base);
    local_ptr_base = nullptr;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::constexpr_set_result const& csr)
{
    this->constexpr_result_v = slot_consume_data(csr.target);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::load_const_value const& lcv)
{
    auto& frame = get_current_frame();
    auto target_type = get_local_type(lcv.target);
    auto output_obj = output(lcv.target);
    auto target_data = constdata(lcv.value);
    auto start_ptr_impl = make_pointer_to(target_data->array_members.at(0));
    auto end_ptr_impl = pointer_arith(start_ptr_impl, lcv.value.size(), void_type{});
    auto start_ptr_object = output_obj->struct_members["__start"];
    auto end_ptr_object = output_obj->struct_members["__end"];
    begin_lifetime(start_ptr_object);
    start_ptr_object->ref = start_ptr_impl;
    begin_lifetime(end_ptr_object);
    end_ptr_object->ref = end_ptr_impl;
    begin_lifetime(output_obj);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::make_pointer_to const& mpt)
{
    auto& frame = get_current_frame();
    auto& source = frame.local_values.at(mpt.of_index);
    if (!source || !source->ref.has_value())
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
        std::cout << "ptr object id: " << ptr->object_id << std::endl;
        throw std::logic_error("pointer missing value?");
    }

    auto pointer_target = ptr->ref.value().pointer_target.value().lock();

    if (!pointer_target)
    {
        throw std::logic_error("invalid 2");
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

    auto load_from_ptr = slot->ref->pointer_target.value().lock();
    slot = nullptr;
    if (!load_from_ptr)
    {
        throw constexpr_logic_execution_error("Error executing <load_from_ref>: accessing deallocated storage");
    }
    auto& load_from = *load_from_ptr;

    auto target_slot = output_local(lfr.to_value);
    target_slot->data = load_from.data;
    target_slot->ref = load_from.ref;
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
    require_valid_input_precondition(sub.a);
    require_valid_input_precondition(sub.b);
    require_valid_output_precondition(sub.result);

    auto a = consume_local(sub.a);
    auto b = consume_local(sub.b);
    auto r = output_local(sub.result);

    // Retrieve data references
    auto& a_data = a->data;
    auto& b_data = b->data;
    auto& r_data = r->data;

    // Ensure sizes match
    if (a_data.size() != b_data.size())
        throw std::runtime_error("int_sub: 'a' and 'b' have different sizes");
    if (r_data.size() != a_data.size())
        r_data.resize(a_data.size());

    // We'll create a temporary buffer for -b
    std::vector< std::byte > neg_b = b_data;

    // Compute ~b (bitwise NOT)
    for (auto& byte : neg_b)
    {
        byte = static_cast< std::byte >(~static_cast< std::uint8_t >(byte));
    }

    // Add 1 to ~b to complete two's complement negation
    std::uint16_t carry = 1;
    for (std::size_t i = 0; i < neg_b.size() && carry; i++)
    {
        std::uint16_t val = static_cast< std::uint8_t >(neg_b[i]) + carry;
        neg_b[i] = static_cast< std::byte >(static_cast< std::uint8_t >(val & 0xFF));
        carry = (val > 0xFF) ? 1 : 0;
    }

    // Now perform a + neg_b (which is a - b)
    carry = 0;
    for (std::size_t i = 0; i < r_data.size(); ++i)
    {
        std::uint16_t av = static_cast< std::uint8_t >(a_data[i]);
        std::uint16_t bv = static_cast< std::uint8_t >(neg_b[i]);

        std::uint16_t sum = av + bv + carry;
        r_data[i] = static_cast< std::byte >(static_cast< std::uint8_t >(sum & 0xFF));
        carry = (sum > 0xFF) ? 1 : 0;
    }

    // Overflow (carry after the last byte) simply wraps in two's complement

    return;
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_add const& add)
{
    // Retrieve the current frame to access local values
    auto& frame = get_current_frame();

    std::shared_ptr< local > a_local = consume_local(add.a);
    std::shared_ptr< local > b_local = consume_local(add.b);
    std::shared_ptr< local > r_local = output_local(add.result);

    // Retrieve data references
    auto& a_data = a_local->data;
    auto& b_data = b_local->data;
    auto& r_data = r_local->data;

    // Ensure operands and result have the same size
    // Typically, the IR would ensure the same type/size for these operands,
    // but we'll check anyway.
    if (a_data.size() != b_data.size())
    {
        throw std::runtime_error("int_add: 'a' and 'b' have different sizes");
    }
    if (r_data.size() != a_data.size())
    {
        // If there's a mismatch, we can resize result to match
        // but ideally, all should be consistent via IR definitions.
        r_data.resize(a_data.size());
    }

    type_symbol a_type = get_local_type(add.a);
    type_symbol b_type = get_local_type(add.b);
    type_symbol r_type = get_local_type(add.result);

    if (a_type != b_type || a_type != r_type)
    {
        throw std::runtime_error("int_add: type mismatch among operands");
    }

    if (!typeis< int_type >(a_type))
    {
        throw std::runtime_error("int_add: operands are not of integer type");
    }

    int_type const& int_type_info = a_type.get_as< int_type >();

    bytemath::fixed_int_options opts;
    opts.bits = int_type_info.bits;
    opts.has_sign = int_type_info.has_sign;
    opts.overflow_undefined = opts.has_sign;

    bytemath::int_result res = bytemath::fixed_int_add_le(opts, a_data, b_data);
    // Perform two's complement addition in little-endian order

    if (res.result_is_undefined)
    {
        throw constexpr_logic_execution_error("error executing IADD: undefined behavior");
    }

    assert(r_data.size() == res.data_bytes.size());
    r_data = res.data_bytes;

    return;
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_mul const& mul)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_div const& div)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::int_mod const& mod)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::store_to_ref const& str)
{
    auto from_ptr = get_pointer_to(current_frame_index(), str.from_value);
    auto to_ptr = get_pointer_to(current_frame_index(), str.to_reference);

    // Both values should be alive(?)

    if (from_ptr.pointer_target.value().expired())
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: loading from deallocated storage");
    }
    if (to_ptr.pointer_target.value().expired())
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: storing into deallocated storage");
    }

    auto& from_ptr_target = *from_ptr.pointer_target.value().lock();
    auto& to_ptr_target = *to_ptr.pointer_target.value().lock();

    std::size_t write_size = from_ptr_target.data.size();

    for (auto i = 0; i < write_size; i++)
    {
        auto to_ptr_offset = i;
        if (to_ptr_offset >= to_ptr_target.data.size())
        {
            std::cout << "str: from: " << from_ptr_target.object_id << " to: " << to_ptr_target.object_id << std::endl;
            throw constexpr_logic_execution_error("Error executing <store_to_ref>: out of bounds write");
        }
        to_ptr_target.data[to_ptr_offset] = from_ptr_target.data[i];
    }

    to_ptr_target.ref = from_ptr_target.ref;

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

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::cmp_eq const& ceq)
{
    auto a = slot_consume_data(ceq.a);
    auto b = slot_consume_data(ceq.b);

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
    auto a = slot_consume_data(cne.a);
    auto b = slot_consume_data(cne.b);

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
    auto a = slot_consume_data(clt.a);
    auto b = slot_consume_data(clt.b);

    std::cout << "CLT " << bytemath::detail::le_to_string_raw(a) << " " << bytemath::detail::le_to_string_raw(b) << std::endl;

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
    auto a = slot_consume_data(cge.a);
    auto b = slot_consume_data(cge.b);

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

    // Equal values => a >= b
    set_data(cge.result, {std::byte(1)});
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

void quxlang::vmir2::ir2_constexpr_interpreter::add_functanoid3(type_symbol addr, functanoid_routine3 func)
{
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
        }
    }
    this->implementation->functanoids3[addr] = std::move(func);
    this->implementation->missing_functanoids_val.erase(addr);
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

std::set< quxlang::type_symbol > const& quxlang::vmir2::ir2_constexpr_interpreter::missing_functanoids()
{
    return this->implementation->missing_functanoids_val;
}

std::vector< std::byte > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::slot_consume_data(vmir2::local_index slot)
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
    auto data = slot_consume_data(slot);
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
    // storage_valid becomes true only if this slot is a delegate (i.e., member_of has value),
    // otherwise consuming invalidates the storage.
    result->storage_initiated = result->member_of.has_value();

    // Remove from frame to reflect consumption
    get_current_frame().local_values[slot] = nullptr;

    return result;
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

    for (auto [idx, local] : frame_object.local_values)
    {
        if (local == nullptr)
            continue;
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
        throw compiler_bug("this shouldn't be possible");
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

        if (!is_ref(field_type))
        {
            create_local_value(index, false);
        }

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
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::end_lifetime const& elt)
{
    auto& frame = get_current_frame();

    auto& local_ptr = frame.local_values.at(elt.of);
    end_lifetime(local_ptr);
    local_ptr = nullptr;
}

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::create_local_value(vmir2::local_index local_idx, bool set_alive)
{
    assert(!set_alive);
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

    auto ptr_target = ptr_ref.value().pointer_target.value().lock();

    if (!ptr_target)
    {
        throw constexpr_logic_execution_error("dereferencing invalidated pointer or reference");
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
                bool local_has_nontrivial_dtor = current_func_ir->non_trivial_dtors.contains(slot_type);
                if (local_has_nontrivial_dtor)
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
            }
            else if (new_values.contains(idx))
            {
                // If we have a NEW[T] slot, then returning from this function
                // transitions from partial to full by definition.
                assert(local->stage != slot_stage::dead);
                // Note: array initializers will already construct the object, that's okay.
                begin_lifetime(local);
            }
            else if (local->alive() && !value_should_be_alive(idx))
            {
                auto slot_type = current_func_ir->local_types.at(idx).type;
                bool local_has_nontrivial_dtor = current_func_ir->non_trivial_dtors.contains(slot_type);
                if (local_has_nontrivial_dtor)
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
    exec_instr_val_incdec(instr.value, instr.result, true);
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

    if (object->array_init_member_of.has_value())
    {
        auto array_init = object->array_init_member_of.value().lock();

        array_init->init_count++;
        object->array_init_member_of = std::nullopt;
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::end_lifetime(std::shared_ptr< local > object)
{
    object->stage = slot_stage::dead;
    if (!object->member_of.has_value())
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

std::shared_ptr< quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::local > quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::constdata(std::vector< std::byte > const& data)
{
    auto& cell = this->global_constdata[data];
    if (cell == nullptr)
    {
        array_type constdata_type;
        // TODO: Convert this to byte type instead of U8.
        constdata_type.element_type = int_type{.bits = 8, .has_sign = false};
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
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val_incdec(local_index val, local_index result, bool increment)
{
    auto const& type = frame_slot_data_type(val);

    if (!typeis< ptrref_type >(type))
    {
        throw invalid_instruction_error("Expected reference to int or pointer to increment, but got " + to_string(type) + " instead");
    }

    auto type_int_or_pointer_g = type.get_as< ptrref_type >().target;

    if (typeis< int_type >(type_int_or_pointer_g))
    {
        exec_incdec_int(val, result, increment);
    }
    else if (typeis< ptrref_type >(type_int_or_pointer_g))
    {
        exec_incdec_ptr(val, result, increment);
    }
    else
    {
        throw invalid_instruction_error("Error in [increment]: slot is not an int or pointer");
    }
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_incdec_int(local_index input_slot, local_index output_slot, bool increment)
{
    auto const& type = frame_slot_data_type(input_slot);
    auto type_int_g = type.get_as< ptrref_type >().target;

    int_type const& type_int = type_int_g.get_as< int_type >();

    auto value_to_increment = load_from_reference(input_slot, false);

    auto val = local_consume_data(value_to_increment);

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

    store_as_reference(output_slot, value_to_increment);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_incdec_ptr(local_index input_slot, local_index output_slot, bool increment)
{
    auto value_to_increase = load_from_reference(input_slot, false);

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

    // TODO: include type?
    ptr = pointer_arith(ptr.value(), increment ? 1 : -1, void_type{});

    this->store_as_reference(output_slot, value_to_increase);
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::assert_instr const& asrt)
{
    auto reg = asrt.condition;

    auto data = slot_consume_data(reg);

    if (data == std::vector< std::byte >{std::byte{0}})
    {
        throw constexpr_logic_execution_error("assertion failed: " + asrt.message);
    }
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::decrement const& instr)
{
    exec_instr_val_incdec(instr.value, instr.result, false);
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::preincrement const& instr)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::predecrement const& instr)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::pointer_arith const& par)
{

    auto ptr = load_as_pointer(par.from, true);

    // quxlang::bytemath::le_sint multiplier;

    auto val = slot_consume_data(par.offset);

    auto const& offset_type = get_local_type(par.offset);

    if (!offset_type.type_is< int_type >())
    {
        throw compiler_bug("Error in [pointer_arith]: offset is not an integer");
    }

    auto const& offset_int = offset_type.get_as< int_type >();

    // TODO: Handle signed offsets
    if (offset_int.has_sign)
    {
        throw rpnx::unimplemented();
    }
    auto offset_val = slot_consume_data(par.offset);

    std::int64_t offset = 0;
    bool ok = false;

    if (par.multiplier == -1)
    {
        auto offset_val_s = bytemath::unlimited_int_signed_sub_le(bytemath::sle_int_unlimited{{std::byte(0)}, false}, bytemath::sle_int_unlimited{offset_val, false});
        std::tie(offset, ok) = offset_val_s.to_int< std::int64_t >();
    }
    else
    {
        std::tie(offset, ok) = bytemath::sle_int_unlimited{offset_val, false}.to_int< std::int64_t >();
    }
    // TODO: Handle !ok

    assert(ok);

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

    assert(slot_ptr && slot_ptr->ref);

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
    // Stub implementation for pointer_diff
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_constexpr_interpreter::ir2_constexpr_interpreter_impl::exec_instr_val(vmir2::array_init_start const& ais)
{
    auto& frame = get_current_frame();

    auto array = frame.local_values[ais.on_value];

    auto initializer = output_local(ais.initializer);

    auto initializer_type = get_local_type(ais.initializer);

    if (!initializer_type.template type_is< array_initializer_type >()) [[unlikely]]
    {
        throw invalid_instruction_transition_error("array_init_start expects __ARRAY_INITAILIZER");
    }



    initializer->initializer_of = array;
    assert(array != nullptr);
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

    for (auto& [idx, local] : get_current_frame().local_values)
    {
        if (local && local->array_init_member_of.has_value() && local->array_init_member_of.value().lock() == initializer)
        {
            local->array_init_member_of.reset();
            assert(local->member_of.value().lock() == array);
            local = nullptr;
        }
    }
    begin_lifetime(array);
}
