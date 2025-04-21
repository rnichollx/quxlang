//
// Created by Ryan Nicholl on 2024-12-01.
//

#include <utility>

#include "quxlang/vmir2/ir2_interpreter.hpp"

#include "quxlang/compiler.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/parsers/parse_int.hpp"
#include "quxlang/vmir2/assembly.hpp"
#include "rpnx/value.hpp"

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

class quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl
{
    friend class quxlang::vmir2::ir2_interpreter;

  private:
    std::map< cow< type_symbol >, class_layout > class_layouts;
    std::map< cow< type_symbol >, cow< functanoid_routine2 > > functanoids;
    std::vector< std::byte > constexpr_result;
    std::optional< type_symbol > constexpr_result_type;
    std::optional< type_symbol > constexpr_result_type_value;

    std::set< type_symbol > missing_functanoids_val;

    std::uint64_t next_object_id = 1;

    struct local;

    struct pointer_impl
    {
        std::weak_ptr< local > pointer_target;
    };

    struct local
    {
        std::vector< std::byte > data;
        bool alive = false;
        bool storage_initiated = false;
        bool dtor_enabled = false;
        std::uint64_t object_id{};
        std::optional< pointer_impl > ref;
        std::weak_ptr< local > member_of;

        std::optional< dtor_spec > dtor;

        std::vector< std::shared_ptr< local > > array_members;
        std::map< std::string, std::shared_ptr< local > > struct_members;
    };

    struct stack_frame
    {
        cow< type_symbol > type;
        cow< functanoid_routine2 > ir;
        interp_addr address;

        std::map< std::size_t, std::shared_ptr< local > > local_values;
    };

    std::vector< stack_frame > stack;

    void call_func(cow< type_symbol > functype, vmir2::invocation_args args);
    void exec();

    void exec_instr();

    stack_frame& get_current_frame()
    {
        return stack.back();
    }

    stack_frame& current_frame()
    {
        return stack.back();
    }

    std::size_t current_frame_index()
    {
        return stack.size() - 1;
    }

    std::size_t get_type_size(const type_symbol& type);

    pointer_impl get_pointer_to(std::size_t frame, std::size_t slot);

    void transition(vmir2::block_index block);

    void exec_instr_val(vmir2::increment const& inc);
    void exec_instr_val(vmir2::decrement const& dec);
    void exec_instr_val(vmir2::preincrement const& inc);
    void exec_instr_val(vmir2::predecrement const& dec);
    void exec_instr_val(vmir2::load_const_zero const& lcz);
    void exec_instr_val(vmir2::access_field const& acf);
    void exec_instr_val(vmir2::to_bool const& lcz);
    void exec_instr_val(vmir2::to_bool_not const& acf);
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
    void exec_instr_val(vmir2::defer_nontrivial_dtor const& dntd);
    void exec_instr_val(vmir2::struct_delegate_new const& sdn);
    void exec_instr_val(vmir2::copy_reference const& cpr);
    void exec_instr_val(vmir2::end_lifetime const& elt);
    void exec_instr_val(vmir2::pointer_arith const& par);

    std::shared_ptr< local > create_local_value(std::size_t local_index, bool set_alive);
    void init_storage(std::shared_ptr< local > local_value, type_symbol type);

    std::vector< std::byte > use_data(std::size_t slot);

    std::vector< std::byte > consume_data(std::size_t slot);
    std::vector< std::byte > copy_data(std::size_t slot);

    std::uint64_t consume_u64(std::size_t slot);

    uint64_t alloc_object_id();
    void set_data(std::size_t slot, std::vector< std::byte > data);

    bool consume_bool(std::size_t slot);
    void output_bool(std::size_t slot, bool value);

    type_symbol frame_slot_data_type(std::size_t slot);
};

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::call_func(cow< type_symbol > functype, vmir2::invocation_args args)
{

    // To call a function we push a stack frame onto the stack, copy any relevant arguments, then return.
    // Actual execution will happen in subsequent calls to exec/exec_instr

    std::string funcname_str = quxlang::to_string(functype.get());

    stack.emplace_back();
    stack.back().type = functype;
    stack.back().ir = functanoids.at(functype);
    stack.back().address = {functype, functanoids.at(functype)->entry_block, 0};

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

    auto const& current_func_ir = functanoids.at(functype);

    // Now there should be at minimum 2 frames, the caller frame and the callee frame.
    if (stack.size() < 2)
    {
        throw compiler_bug("Attempt to call function with arguments without a stack frame");
    }

    auto const& previous_func_ir = functanoids.at(stack[stack.size() - 2].type);

    std::size_t arg_count = 0;
    std::size_t positional_arg_id = 0;

    // previous is the caller frame, current is the callee frame
    auto& current_frame = stack.back();
    auto& previous_frame = stack[stack.size() - 2];

    // Look through all slots in the new function for arguments
    for (std::size_t slot_index = 0; slot_index < current_func_ir->slots.size(); slot_index++)
    {
        auto const& slot = current_func_ir->slots[slot_index];

        if (slot.kind != vmir2::slot_kind::named_arg && slot.kind != vmir2::slot_kind::positional_arg)
        {
            continue;
        }

        arg_count++;

        auto handle_arg = [&](vmir2::storage_index previous_arg_index, vmir2::storage_index new_arg_index)
        {
            type_symbol const& arg_type = slot.type;

            std::string arg_type_str = quxlang::to_string(arg_type);

            if (!typeis< nvalue_slot >(arg_type))
            {
                if (!previous_frame.local_values[previous_arg_index]->alive)
                {
                    throw constexpr_logic_execution_error("Error in argument passing: argument is not alive");
                }
            }

            if (typeis< nvalue_slot >(arg_type) || typeis< dvalue_slot >(arg_type))
            {
                // Slots create a shared reference to the same object in a previous frame, unlike all other
                // calls which consume their arguments.

                if (typeis< nvalue_slot >(arg_type))
                {
                    // The existing storage might not exist, allocate it now if so.

                    if (!previous_frame.local_values.contains(previous_arg_index))
                    {
                        previous_frame.local_values[previous_arg_index] = std::make_shared< local >();
                    }
                }

                current_frame.local_values[new_arg_index] = previous_frame.local_values[previous_arg_index];

                if (typeis< dvalue_slot >(arg_type))
                {
                    previous_frame.local_values[previous_arg_index] = nullptr;
                }

                // The caller should have already initialized the local, not here.
                assert(current_frame.local_values[new_arg_index] != nullptr);

                if (typeis< nvalue_slot >(arg_type))
                {
                    // NEW& values should NOT be alive when we enter the function,
                    // and are alive when we return except via exception
                    assert(!current_frame.local_values[new_arg_index]->alive);
                }

                if (typeis< dvalue_slot >(arg_type))
                {
                    // DESTROY& values should be alive when we enter the function
                    // and are not alive when we return OR throw an exception
                    assert(current_frame.local_values[new_arg_index]->alive);
                }
            }

            else
            {
                assert(!typeis< nvalue_slot >(arg_type) && !typeis< dvalue_slot >(arg_type));
                // In all other cases, the argument is consumed by the call,

                current_frame.local_values[new_arg_index] = previous_frame.local_values[previous_arg_index];
                previous_frame.local_values[previous_arg_index] = nullptr;

                // TODO: If the type is trivially relocatable, invalidate all prior references to it now
                // We don't currently pass this information, so this is a TODO.
            }
        };

        if (slot.kind == vmir2::slot_kind::named_arg)
        {
            auto const& arg_name = slot.name;

            assert(arg_name.has_value());

            if (!args.named.contains(*arg_name))
            {
                throw compiler_bug("Missing named argument");
            }

            auto previous_arg_index = args.named.at(*arg_name);
            auto new_arg_index = slot_index;

            handle_arg(previous_arg_index, new_arg_index);
        }
        else if (slot.kind == vmir2::slot_kind::positional_arg)
        {
            if (positional_arg_id >= args.positional.size())
            {
                throw compiler_bug("Missing positional argument");
            }

            auto previous_arg_index = args.positional.at(positional_arg_id);

            positional_arg_id++;
            auto new_arg_index = slot_index;

            handle_arg(previous_arg_index, new_arg_index);
        }
    }

    // We should have gone through all args at this point.
    assert(arg_count == args.size());
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec()
{
    // print all functanoids

    for (auto const& func : functanoids)
    {
        quxlang::vmir2::assembler ir_printer(func.second.get());

        std::cout << "Functanoid: " << quxlang::to_string(*(func.first)) << std::endl;

        std::cout << ir_printer.to_string(func.second.get()) << std::endl;
    }

    while (!stack.empty())
    {
        exec_instr();
    }
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr()
{
    interp_addr& current_instr_address = stack.back().address;

    auto const& current_func = stack.back().type;

    auto const& current_func_ir = functanoids.at(current_func.get());

    auto const& current_block = current_func_ir->blocks.at(current_instr_address.block);

    if (current_instr_address.instruction_index < current_block.instructions.size())
    {
        vm_instruction const& instr = current_block.instructions.at(current_instr_address.instruction_index);
        quxlang::vmir2::assembler ir_printer(current_func_ir.get());

        std::cout << "Executi   ng in constexpr " << quxlang::to_string(current_func.get()) << " block " << current_instr_address.block << " instruction " << current_instr_address.instruction_index << ": " << ir_printer.to_string(instr) << std::endl;
        // If there is an error here, it usually means there is an instruction which is not implemented
        // on the constexpr virtual machine. Instructions which are illegal in a constexpr context
        // should be implemented to throw a derivative of std::logic_error.
        rpnx::apply_visitor< void >(
            [this](auto const& param)
            {
                return this->exec_instr_val(param);
            },
            instr);
        current_instr_address.instruction_index++;
        return;
    }

    auto const terminator_instruction = current_block.terminator;
    if (!terminator_instruction)
    {
        throw constexpr_logic_execution_error("Constexpr execution reached end of block with undefined behavior");
    }

    rpnx::apply_visitor< void >(
        [this](auto const& param)
        {
            return this->exec_instr_val(param);
        },
        *terminator_instruction);
    return;
}

std::size_t quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::get_type_size(const type_symbol& type)
{

    std::string type_str = quxlang::to_string(type);

    if (typeis< int_type >(type))
    {
        return (type.get_as< int_type >().bits + 7) / 8;
    }

    if (typeis< bool_type >(type))
    {
        return 1;
    }

    // Pointer data is stored in ref, not data bytes.
    if (typeis< pointer_type >(type))
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

quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::pointer_impl quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::get_pointer_to(std::size_t frame, std::size_t slot)
{
    // This can either be a value or a reference,

    bool is_ref = quxlang::is_ref(stack[frame].ir->slots.at(slot).type);

    if (!is_ref)
    {
        pointer_impl result;
        result.pointer_target = stack[frame].local_values[slot];
        if (result.pointer_target.lock() == nullptr)
        {
            throw compiler_bug("Attempt to take reference to non-existant storage location");
        }

        return result;
    }
    else
    {
        auto ptr = stack[frame].local_values[slot]->ref;
        assert(ptr.has_value());
        if (ptr->pointer_target.lock() == nullptr)
        {
            throw compiler_bug("Attempt to create pointer to non-extant storage location");
        }
        return *ptr;
    }
}


void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::load_const_zero const& lcz)
{
    auto const& type = get_current_frame().ir->slots.at(lcz.target).type;

    auto sz = get_type_size(type);

    if (is_ref(type))
    {
        throw compiler_bug("Cannot load zero into reference");
    }

    if (get_current_frame().local_values[lcz.target] && get_current_frame().local_values[lcz.target]->alive)
    {
        throw compiler_bug("Local value already has pointer");
    }

    auto local_ptr = create_local_value(lcz.target, true);
    init_storage(local_ptr, type);

    std::cout << "lcz: " << local_ptr->object_id << std::endl;

    return;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::access_field const& acf)
{
    auto& frame = get_current_frame();
    auto& parent_ref_slot = frame.local_values[acf.base_index];

    if (parent_ref_slot == nullptr || !parent_ref_slot->ref.has_value())
    {
        throw compiler_bug("shouldn't be possible");
    }

    auto ref_to_ptr = parent_ref_slot->ref.value().pointer_target.lock();

    if (ref_to_ptr == nullptr)
    {
        // TODO: this isn't a compiler bug but rather a coding bug
        throw compiler_bug("accessing field of object that does not have field");
    }

    auto& field = frame.local_values[acf.store_index];
    if (field != nullptr)
    {
        throw compiler_bug("trying to load a field into existing slot");
    }

    field = std::make_shared< local >();

    auto& field_slot = ref_to_ptr->struct_members.at(acf.field_name);

    field->ref = pointer_impl{.pointer_target = field_slot};
    field->alive = true;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::to_bool const& tb)
{
    // There are two different versions of TB instruction, one which operates on pointer and one which
    // operates on integers.
    // These work the same on real machines, but because we treat pointers specially in constexpr, we
        // need to handle them differently.

    auto typ = frame_slot_data_type(tb.from);

    if (typ.type_is< pointer_type >() && !(typ.get_as<pointer_type>().ptr_class == pointer_class::ref))
    {
        auto &local = get_current_frame().local_values[tb.from];

        if (local == nullptr)
        {
            // Maybe invalid instruction? but this should have been caught earlier
            // throw invalid_instruction_transition_error("missing slot");
            throw compiler_bug("slot missing");
        }

        if (!local->alive)
        {
            throw constexpr_logic_execution_error("Error executing <to_bool>: accessing deallocated object");
        }

        output_bool(tb.to, local->ref.has_value());
    }
    else
    {
        auto data = consume_data(tb.from);

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

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::to_bool_not const& tbn)
{
    // There are two different versions of TB instruction, one which operates on pointer and one which
    // operates on integers.
    // These work the same on real machines, but because we treat pointers specially in constexpr, we
    // need to handle them differently.

    auto typ = frame_slot_data_type(tbn.from);

    if (typ.type_is< pointer_type >() && !(typ.template as<pointer_type>().ptr_class == pointer_class::ref))
    {
        auto &local = get_current_frame().local_values[tbn.from];

        if (local == nullptr)
        {
            // Maybe invalid instruction? but this should have been caught earlier
            // throw invalid_instruction_transition_error("missing slot");
            throw compiler_bug("slot missing");
        }

        if (!local->alive)
        {
            throw constexpr_logic_execution_error("Error executing <to_bool>: accessing deallocated object");
        }

        output_bool(tbn.to, !local->ref.has_value());
    }
    else
    {
        auto data = consume_data(tbn.from);

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
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::access_array const& aca)
{
    auto& frame = get_current_frame();
    auto& parent_ref_slot = frame.local_values[aca.base_index];

    if (parent_ref_slot == nullptr || !parent_ref_slot->ref.has_value())
    {
        throw compiler_bug("shouldn't be possible");
    }

    auto ref_to_ptr = parent_ref_slot->ref.value().pointer_target.lock();

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

    field = std::make_shared< local >();

    auto arry_index = consume_u64(aca.index_index);

    auto& field_slot = ref_to_ptr->array_members.at(arry_index);

    field->ref = pointer_impl{.pointer_target = field_slot};
    field->alive = true;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::invoke const& inv)
{
    call_func(inv.what, inv.args);
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::make_reference const& mrf)
{
    auto ptr = get_pointer_to(current_frame_index(), mrf.value_index);

    auto ref_type = get_current_frame().ir->slots.at(mrf.reference_index).type;
    if (!is_ref(ref_type))
    {
        throw compiler_bug("Attempt to make reference by non-reference type");
    }

    if (get_current_frame().local_values[mrf.reference_index])
    {
        throw compiler_bug("Attempt to overwrite reference (A)");
    }

    auto& local_ptr = get_current_frame().local_values[mrf.reference_index];
    if (local_ptr)
    {
        throw compiler_bug("Attempt to overwrite reference (B)");
    }

    local_ptr = std::make_shared< local >();
    local_ptr->ref = ptr;
    local_ptr->alive = true;

    return;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::jump const& jmp)
{
    transition(jmp.target);
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::branch const& brn)
{
    auto reg = brn.condition;

    auto data = consume_data(reg);

    if (data == std::vector< std::byte >{std::byte{0}})
    {
        transition(brn.target_false);
    }
    else
    {
        transition(brn.target_true);
    }
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cast_reference const& cst)
{
    auto& local_ptr_base = get_current_frame().local_values.at(cst.source_ref_index);
    auto& local_ptr_result = get_current_frame().local_values[cst.target_ref_index];

    if (!local_ptr_base)
    {
        throw constexpr_logic_execution_error("Error executing <cast_reference>: accessing deallocated storage");
    }

    if (!local_ptr_base->alive)
    {
        throw constexpr_logic_execution_error("Error executing <cast_reference>: accessing dealived storage");
    }

    if (local_ptr_result && local_ptr_result->alive)
    {
        throw compiler_bug("Attempt to overwrite local value");
    }

    local_ptr_result = std::make_shared< local >();

    local_ptr_result->ref = local_ptr_base->ref;
    local_ptr_result->alive = true;

    local_ptr_base->alive = false;

    local_ptr_base = nullptr;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::constexpr_set_result const& csr)
{
    auto& frame = get_current_frame();

    auto& slot_info = frame.ir.read().slots;
    if (slot_info.at(csr.target).kind == slot_kind::literal)
    {
        if (slot_info.at(csr.target).type.template type_is< numeric_literal_reference >())
        {
            // Special case, we can set the result directly as if it were U64
            auto literal_value = slot_info.at(csr.target).literal_value.value();
            std::uint64_t result = 0;

            while (literal_value.empty() == false)
            {
                if (std::numeric_limits< std::uint64_t >::max() / 10 < result)
                {
                    throw std::logic_error("Overflow in numeric literal to U64 conversion");
                }
                result *= 10;
                if (std::numeric_limits< std::uint64_t >::max() - (literal_value.front() - '0') < result)
                {
                    throw std::logic_error("Overflow in numeric literal to U64 conversion");
                }
                result += (literal_value.front() - '0');

                literal_value.erase(literal_value.begin(), literal_value.begin() + 1);
            }

            while (result > 0)
            {
                this->constexpr_result.push_back(static_cast< std::byte >(result & 0xFF));
                result >>= 8;
            }

            while (this->constexpr_result.size() < 8)
            {
                this->constexpr_result.push_back(std::byte{0});
            }

            return;
        }
    }

    this->constexpr_result = consume_data(csr.target);
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::load_const_value const& lcv)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::make_pointer_to const& mpt)
{
    auto& frame = get_current_frame();
    auto& source = frame.local_values.at(mpt.of_index);
    if (!source || !source->ref.has_value())
    {
        throw constexpr_logic_execution_error("Expected source value to be valid");
    }
    pointer_impl ptr = get_pointer_to(stack.size() - 1, mpt.of_index);
    if (ptr.pointer_target.expired())
    {
        throw std::logic_error("creating a pointer to non value???");
    }
    auto ptrval = create_local_value(mpt.pointer_index, true);
    ptrval->ref = ptr;
    std::cout << "make_pointer_to: des object id: " << ptrval->object_id << std::endl;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::dereference_pointer const& drp)
{
    auto& frame = get_current_frame();

    auto ptr = frame.local_values.at(drp.from_pointer);

    std::shared_ptr< local >& ref = frame.local_values[drp.to_reference];

    if (ref != nullptr)
    {
        throw std::logic_error("invalid");
    }

    ref = std::make_shared< local >();

    if (!ptr->ref.has_value())
    {
        std::cout << "ptr object id: " << ptr->object_id << std::endl;
        throw std::logic_error("pointer missing value?");
    }

    auto pointer_target = ptr->ref.value().pointer_target.lock();

    if (!pointer_target)
    {
        throw std::logic_error("invalid 2");
    }

    pointer_impl pointer_to_target = pointer_impl{.pointer_target = pointer_target};

    ref->ref = pointer_to_target;
    ref->alive = true;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::load_from_ref const& lfr)
{
    auto& slot = get_current_frame().local_values.at(lfr.from_reference);

    if (!slot || !slot->alive)
    {
        throw constexpr_logic_execution_error("Error executing <load_from_ref>: accessing deallocated storage");
    }
    assert(slot->ref.has_value());
    auto load_from_ptr = slot->ref->pointer_target.lock();
    if (!load_from_ptr)
    {
        throw constexpr_logic_execution_error("Error executing <load_from_ref>: accessing deallocated storage");
    }
    auto& load_from = *load_from_ptr;
    auto& target_slot = get_current_frame().local_values[lfr.to_value];
    if (target_slot == nullptr)
    {
        target_slot = std::make_shared< local >();
    }
    target_slot->data = load_from.data;
    target_slot->ref = load_from.ref;
    target_slot->alive = true;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::ret const& ret)
{
    // TODO: Run destructors
    stack.pop_back();
    if (stack.size() >= 1)
    {
        get_current_frame().address.instruction_index++;
    }
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::int_add const& add)
{
    // Retrieve the current frame to access local values
    auto& frame = get_current_frame();

    // Initialize the result local if it does not exist
    if (!frame.local_values.count(add.result))
    {
        // Create a new local for the result
        create_local_value(add.result, true);
    }

    // Retrieve data references
    auto& a_data = frame.local_values.at(add.a)->data;
    auto& b_data = frame.local_values.at(add.b)->data;
    auto& r_data = frame.local_values.at(add.result)->data;

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



    // Perform two's complement addition in little-endian order
    std::uint16_t carry = 0;
    for (std::size_t i = 0; i < r_data.size(); ++i)
    {
        std::uint16_t av = static_cast< std::uint8_t >(a_data[i]);
        std::uint16_t bv = static_cast< std::uint8_t >(b_data[i]);

        std::uint16_t sum = av + bv + carry;
        r_data[i] = static_cast< std::byte >(static_cast< std::uint8_t >(sum & 0xFF));
        carry = (sum > 0xFF) ? 1 : 0;
    }

    // Any leftover carry beyond the last byte effectively wraps around in two's complement arithmetic.
    return;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::int_sub const& sub)
{
    auto& frame = get_current_frame();

    // Initialize the result local if it does not exist
    if (!frame.local_values.count(sub.result))
    {
        // Retrieve the type of the result slot
        create_local_value(sub.result, true);
    }

    // Retrieve data references
    auto& a_data = frame.local_values.at(sub.a)->data;
    auto& b_data = frame.local_values.at(sub.b)->data;
    auto& r_data = frame.local_values.at(sub.result)->data;

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
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::int_mul const& mul)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::int_div const& div)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::int_mod const& mod)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::store_to_ref const& str)
{
    auto from_ptr = get_pointer_to(current_frame_index(), str.from_value);
    auto to_ptr = get_pointer_to(current_frame_index(), str.to_reference);

    // Both values should be alive(?)

    if (from_ptr.pointer_target.expired())
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: loading from deallocated storage");
    }
    if (to_ptr.pointer_target.expired())
    {
        throw constexpr_logic_execution_error("Error executing <store_to_ref>: storing into deallocated storage");
    }

    auto& from_ptr_target = *from_ptr.pointer_target.lock();
    auto& to_ptr_target = *to_ptr.pointer_target.lock();

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
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::load_const_int const& lci)
{
    if (!get_current_frame().local_values.contains(lci.target))
    {
        get_current_frame().local_values[lci.target] = std::make_shared< local >();
    }
    auto int_type = get_current_frame().ir->slots.at(lci.target).type;

    if (int_type.type_is< nvalue_slot >())
    {
        auto copy = int_type.get_as< nvalue_slot >().target;

        int_type = copy;
    }
    auto& data = get_current_frame().local_values[lci.target]->data;
    data.resize(get_type_size(int_type));

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

    get_current_frame().local_values[lci.target]->object_id = alloc_object_id();
    get_current_frame().local_values[lci.target]->alive = true;

    return;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cmp_eq const& ceq)
{
    auto a = consume_data(ceq.a);
    auto b = consume_data(ceq.b);

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
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cmp_ne const& cne)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cmp_lt const& clt)
{
    auto a = consume_data(clt.a);
    auto b = consume_data(clt.b);

    for (std::size_t i = a.size() - 1; true; i--)
    {
        if (a[i] < b[i])
        {
            set_data(clt.result, {std::byte(1)});
            return;
        }
        if (a[i] > b[i])
        {
            set_data(clt.result, {std::byte(0)});
            return;
        }
        if (i == 0)
        {
            break;
        }
    }

    set_data(clt.result, {std::byte(0)});
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cmp_ge const& cge)
{
    throw rpnx::unimplemented();
}

quxlang::vmir2::ir2_interpreter::ir2_interpreter()
{
    this->implementation = new ir2_interpreter_impl();
}

quxlang::vmir2::ir2_interpreter::~ir2_interpreter()
{
    delete this->implementation;
    this->implementation = nullptr;
}

void quxlang::vmir2::ir2_interpreter::add_class_layout(quxlang::type_symbol name, quxlang::class_layout layout)
{
    this->implementation->class_layouts[name] = std::move(layout);
}

void quxlang::vmir2::ir2_interpreter::add_functanoid(quxlang::type_symbol addr, quxlang::vmir2::functanoid_routine2 func)
{
    for (auto const& dtor : func.non_trivial_dtors)
    {
        auto dtor_func = dtor.second;

        if (!this->implementation->functanoids.contains(dtor_func))
        {
            this->implementation->missing_functanoids_val.insert(dtor_func);
        }
    }
    for (auto const& block : func.blocks)
    {
        for (auto const& instr : block.instructions)
        {
            if (typeis< vmir2::invoke >(instr))
            {
                auto const& inv = instr.get_as< vmir2::invoke >();

                auto called_func = inv.what;

                if (!this->implementation->functanoids.contains(called_func))
                {
                    this->implementation->missing_functanoids_val.insert(called_func);
                }
            }
        }
    }
    this->implementation->functanoids[addr] = std::move(func);
    this->implementation->missing_functanoids_val.erase(addr);
}

void quxlang::vmir2::ir2_interpreter::exec(type_symbol func)
{
    this->implementation->call_func(func, {});
    this->implementation->exec();
}
bool quxlang::vmir2::ir2_interpreter::get_cr_bool()
{
    assert(this->implementation->constexpr_result.size() == 1);
    return this->implementation->constexpr_result == std::vector< std::byte >{std::byte(1)};
}
std::uint64_t quxlang::vmir2::ir2_interpreter::get_cr_u64()
{
    std::uint64_t result = 0;
    if (this->implementation->constexpr_result.size() != 8)
    {
        throw std::logic_error("expected uint64");
    }
    for (std::size_t i = 0; i < 8; i++)
    {
        result |= (static_cast< std::uint64_t >(static_cast< std::uint8_t >(this->implementation->constexpr_result[i])) << (i * 8));
    }
    return result;
}

std::set< quxlang::type_symbol > const& quxlang::vmir2::ir2_interpreter::missing_functanoids()
{
    return this->implementation->missing_functanoids_val;
}

std::vector< std::byte > quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::consume_data(std::size_t slot)
{
    auto& frame = get_current_frame();

    auto it = frame.local_values.find(slot);
    if (it == frame.local_values.end() || !it->second)
    {
        throw constexpr_logic_execution_error("Error in [consume_data] substep: slot does not exist");
    }

    // Move the data out of the local
    std::vector< std::byte > data = std::move(it->second->data);

    // Reset the slot to nullptr
    it->second.reset();

    return data;
}

std::vector< std::byte > quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::copy_data(std::size_t slot)
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
std::uint64_t quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::consume_u64(std::size_t slot)
{
    auto data = consume_data(slot);
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
uint64_t quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::alloc_object_id()
{
    return next_object_id++;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::set_data(std::size_t slot, std::vector< std::byte > data)
{
    auto& frame = get_current_frame();

    auto& local_ptr = frame.local_values[slot];
    if (local_ptr == nullptr)
    {
        local_ptr = std::make_shared< local >();
    }
    auto& local = *local_ptr;
    local.data = std::move(data);
    local.object_id = alloc_object_id();
    local.alive = true;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::output_bool(std::size_t slot_index, bool value)
{
    auto& frame = get_current_frame();

    auto slot_type = frame_slot_data_type(slot_index);
    if (slot_type != bool_type{})
    {
        throw invalid_instruction_error("Error in [output_bool]: slot is not a boolean");
    }

    auto& slot = frame.local_values[slot_index];
    slot = std::make_shared< local >();

    init_storage(slot, bool_type{});

    set_data(slot_index, {value ? std::byte(1) : std::byte(0)});
}
quxlang::type_symbol quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::frame_slot_data_type(std::size_t slot)
{
    auto& frame = get_current_frame();

    auto const& ir = frame.ir.read();
    if (ir.slots.at(slot).kind != slot_kind::local)
    {
        throw invalid_instruction_error("Error in [frame_slot_data_type]: slot is not a local");
    }

    return ir.slots.at(slot).type;
}

bool quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::consume_bool(std::size_t slot)
{
    auto& frame = get_current_frame();

    auto& local_ptr = frame.local_values[slot];

    if (local_ptr == nullptr)
    {
        throw invalid_instruction_transition_error("Error in [consume_bool]: slot does not exist");
    }

    if (!local_ptr->alive)
    {
        throw invalid_instruction_transition_error("Error in [consume_bool]: slot is not alive");
    }

    auto& slot_val = frame.ir.read().slots.at(slot);
    if (slot_val.kind != slot_kind::local)
    {
        throw invalid_instruction_error("Error in [consume_bool]: slot is not a local");
    }

    if (slot_val.type.type_is< bool_type >())
    {
        throw invalid_instruction_error("Error in [consume_bool]: slot is not a boolean");
    }

    if (local_ptr->data.size() != 1)
    {
        throw compiler_bug("Error in [consume_bool]: boolean slot has invalid size");
    }

    auto result = local_ptr->data[0] != std::byte(0);

    local_ptr->alive = false;
    local_ptr = nullptr;
    return result;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(quxlang::vmir2::defer_nontrivial_dtor const& dntd)
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
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::struct_delegate_new const& sdn)
{
    auto& frame = get_current_frame();

    auto& slot = frame.local_values[sdn.on_value];

    if (slot == nullptr)
    {
        throw compiler_bug("this shouldn't be possible");
    }

    slot->alive = true;
    slot->storage_initiated = true;
    slot->dtor_enabled = false;

    // TODO: May need to implement SDN on arrays?

    assert(sdn.fields.positional.size() == 0);

    for (auto& [name, index] : sdn.fields.named)
    {
        auto const& field_type = frame.ir.get().slots.at(index).type;
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
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::copy_reference const& cpr)
{
    auto ptr = get_pointer_to(current_frame_index(), cpr.from_index);

    auto ref_type = get_current_frame().ir->slots.at(cpr.to_index).type;
    if (!is_ref(ref_type))
    {
        throw compiler_bug("Attempt to make reference by non-reference type");
    }

    if (get_current_frame().local_values[cpr.to_index])
    {
        throw compiler_bug("Attempt to overwrite reference (A)");
    }

    auto& local_ptr = get_current_frame().local_values[cpr.to_index];
    if (local_ptr)
    {
        throw compiler_bug("Attempt to overwrite reference (B)");
    }

    local_ptr = std::make_shared< local >();
    local_ptr->ref = ptr;
    local_ptr->alive = true;

    return;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::end_lifetime const& elt)
{
    auto& frame = get_current_frame();

    auto& local_ptr = frame.local_values.at(elt.of);
    local_ptr->alive = false;
    local_ptr = nullptr;
}
std::shared_ptr< quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::local > quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::create_local_value(std::size_t local_index, bool set_alive)
{
    auto& frame = get_current_frame();

    auto result_type = frame.ir->slots.at(local_index).type;

    frame.local_values[local_index] = std::make_shared< local >();
    frame.local_values[local_index]->object_id = next_object_id++;
    auto& r_data = frame.local_values[local_index]->data;
    r_data.resize(get_type_size(result_type));

    // Initialize the memory to zero just to have a defined state
    std::fill(r_data.begin(), r_data.end(), std::byte(0));

    frame.local_values[local_index]->storage_initiated = true;

    if (set_alive)
    {
        frame.local_values[local_index]->alive = true;
    }

    return frame.local_values[local_index];
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::init_storage(std::shared_ptr< local > local_value, type_symbol type)
{
    local_value->storage_initiated = true;

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
                                local_value->array_members.at(i) = std::make_shared< local >();
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
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::transition(quxlang::vmir2::block_index block)
{
    std::vector< vmir2::storage_index > values_to_destroy;

    auto& current_frame = get_current_frame();
    auto const& current_func_ir = current_frame.ir;

    auto const& target_block = current_func_ir->blocks.at(block);

    std::set< vmir2::storage_index > current_values;
    std::set< vmir2::storage_index > entry_values;

    for (auto& [idx, local] : current_frame.local_values)
    {
        if (local != nullptr)
        {
            if (local->alive && !target_block.entry_state.contains(idx))
            {
                auto slot_type = current_func_ir->slots.at(idx).type;
                bool local_has_nontrivial_dtor = current_func_ir->non_trivial_dtors.contains(slot_type);
                if (local_has_nontrivial_dtor)
                {
                    auto dtor = current_func_ir->non_trivial_dtors.at(slot_type);
                    call_func(dtor, {.named = {{"THIS", idx}}});
                    return;
                }
                else
                {
                    local = nullptr;
                }
            }
        }

        if (target_block.entry_state.contains(idx) && (local == nullptr || local->alive == false))
        {
            throw compiler_bug("Error in [transition]: slot not alive");
        }
    }

    current_frame.address.block = block;
    current_frame.address.instruction_index = 0;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::increment const& instr)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::decrement const& instr)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::preincrement const& instr)
{
    throw rpnx::unimplemented();
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::predecrement const& instr)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::pointer_arith const& par)
{
    throw rpnx::unimplemented();
}
std::vector< std::byte > quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::use_data(std::size_t slot)
{
    auto slot_ptr = get_current_frame().local_values[slot];

    if (slot_ptr == nullptr)
    {
        throw compiler_bug("Error in [use_data]: slot not allocated");
    }

    if (!slot_ptr->alive)
    {
        throw compiler_bug("Error in [use_data]: slot not alive");
    }

    assert(slot_ptr && slot_ptr->ref);

    if (slot_ptr->ref.has_value())
    {
        auto& ref = *slot_ptr->ref->pointer_target.lock();
        return ref.data;
    }
    else
    {
        return slot_ptr->data;
    }
}

