//
// Created by Ryan Nicholl on 2024-12-01.
//

#include <utility>

#include "quxlang/vmir2/ir2_interpreter.hpp"

#include "quxlang/compiler.hpp"
#include "quxlang/exception.hpp"
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
    std::uint64_t next_object_id = 1;

    struct local;

    struct pointer_impl
    {
        std::weak_ptr< local > pointer_target;
        std::size_t local_offset = 0;
    };

    struct local
    {
        std::vector< std::byte > data;
        bool alive = false;
        std::uint64_t object_id{};
        pointer_impl ref;
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

    std::size_t current_frame_index()
    {
        return stack.size() - 1;
    }

    std::size_t get_type_size(const type_symbol& type);

    pointer_impl get_pointer_to(std::size_t frame, std::size_t slot);

    void exec_instr_val(vmir2::load_const_zero const& lcz);
    void exec_instr_val(vmir2::access_field const& acf);
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
    std::vector< std::byte > consume_data(std::size_t slot);
    uint64_t alloc_object_id();
    void set_data(std::size_t slot, std::vector< std::byte > data);
};

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::call_func(cow< type_symbol > functype, vmir2::invocation_args args)
{
    stack.emplace_back();
    stack.back().type = functype;
    stack.back().ir = functanoids.at(functype);
    stack.back().address = {functype, functanoids.at(functype)->entry_block, 0};
    // TODO: Args

    if (functype == void_type{})
    {
        return;
    }

    auto const& current_func_ir = functanoids.at(functype);

    // TODO: Args
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec()
{
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
    if (typeis< int_type >(type))
    {
        return (type.get_as< int_type >().bits + 7) / 8;
    }

    if (typeis< bool_type >(type))
    {
        return 1;
    }

    if (typeis< pointer_type >(type))
    {
        return 8;
    }

    if (this->class_layouts.contains(type))
    {
        return this->class_layouts.at(type).size;
    }

    throw compiler_bug("Attempt to get size of unknown type");
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

        result.local_offset = 0;
        return result;
    }
    else
    {
        auto ptr = stack[frame].local_values[slot]->ref;
        if (ptr.pointer_target.lock() == nullptr)
        {
            throw compiler_bug("Attempt to create pointer to non-extant storage location");
        }
        return ptr;
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

    if (get_current_frame().local_values[lcz.target])
    {
        throw compiler_bug("Local value already has pointer");
    }

    auto& local_ptr = get_current_frame().local_values[lcz.target];

    local_ptr = std::make_shared< local >();
    local_ptr->data.resize(0);
    local_ptr->data.resize(sz);
    local_ptr->object_id = next_object_id++;

    return;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::access_field const& acf)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::invoke const& inv)
{
    throw rpnx::unimplemented();
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

    return;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::jump const& jmp)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::branch const& brn)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cast_reference const& cst)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::constexpr_set_result const& csr)
{
    this->constexpr_result = consume_data(csr.target);
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::load_const_value const& lcv)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::make_pointer_to const& mpt)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::dereference_pointer const& drp)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::load_from_ref const& lfr)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::ret const& ret)
{

    // TODO: Run destructors
    stack.pop_back();
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::int_add const& add)
{
    // Retrieve the current frame to access local values
    auto& frame = get_current_frame();

    // Initialize the result local if it does not exist
    if (!frame.local_values.count(add.result))
    {
        // Retrieve the type of the result slot
        auto result_type = frame.ir->slots.at(add.result).type;

        // Create a new local for the result
        frame.local_values[add.result] = std::make_shared< local >();
        auto& r_data = frame.local_values[add.result]->data;
        r_data.resize(get_type_size(result_type));

        // Initialize the memory to zero just to have a defined state
        std::fill(r_data.begin(), r_data.end(), std::byte(0));
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
        auto result_type = frame.ir->slots.at(sub.result).type;

        // Create a new local for the result
        frame.local_values[sub.result] = std::make_shared< local >();
        auto& r_data = frame.local_values[sub.result]->data;
        r_data.resize(get_type_size(result_type));

        // Initialize the memory to zero
        std::fill(r_data.begin(), r_data.end(), std::byte(0));
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
        auto to_ptr_offset = to_ptr.local_offset + i;
        if (to_ptr_offset >= to_ptr_target.data.size())
        {
            throw constexpr_logic_execution_error("Error executing <store_to_ref>: out of bounds write");
        }
        to_ptr_target.data[to_ptr_offset] = from_ptr_target.data[i];
    }

    return;
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::load_const_int const& lci)
{
    get_current_frame().local_values[lci.target] = std::make_shared< local >();
    auto int_type = get_current_frame().ir->slots.at(lci.target).type;
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

    return;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cmp_eq const& ceq)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cmp_ne const& cne)
{
    throw rpnx::unimplemented();
}
void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr_val(vmir2::cmp_lt const& clt)
{
    auto a = consume_data(clt.a);
    auto b = consume_data(clt.b);

    for (std::size_t i = a.size()-1; true; i--)
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
    this->implementation->functanoids[addr] = std::move(func);
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
uint64_t quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::alloc_object_id()
{
    return next_object_id++;
}

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::set_data(std::size_t slot, std::vector< std::byte > data)
{
    auto& frame = get_current_frame();

    auto it = frame.local_values.find(slot);

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