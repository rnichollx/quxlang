//
// Created by Ryan Nicholl on 2024-12-01.
//

#include <utility>

#include "quxlang/vmir2/ir2_interpreter.hpp"
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
    std::map< type_symbol, class_layout > class_layouts;
    std::map< type_symbol, functanoid_routine2 > functanoids;

    struct stack_frame
    {
        cow< type_symbol > type;
        interp_addr address;
    };

    std::vector< stack_frame > stack;

    void call_func(type_symbol functype, vmir2::invocation_args args);

    bool exec_instr();

    bool exec_instr_val(vmir2::load_const_zero const& lcz);
    bool exec_instr_val(vmir2::access_field const& acf);
    bool exec_instr_val(vmir2::invoke const& inv);
    bool exec_instr_val(vmir2::make_reference const& mrf);
    bool exec_instr_val(vmir2::jump const& jmp);
    bool exec_instr_val(vmir2::branch const& brn);
    bool exec_instr_val(vmir2::cast_reference const& cst);
    bool exec_instr_val(vmir2::constexpr_set_result const& csr);
    bool exec_instr_val(vmir2::load_const_value const& lcv);
    bool exec_instr_val(vmir2::make_pointer_to const& mpt);
    bool exec_instr_val(vmir2::dereference_pointer const& drp);
    bool exec_instr_val(vmir2::load_from_ref const& lfr);
    bool exec_instr_val(vmir2::ret const& ret);
    bool exec_instr_val(vmir2::int_add const& add);
    bool exec_instr_val(vmir2::int_sub const& sub);
    bool exec_instr_val(vmir2::int_mul const& mul);
    bool exec_instr_val(vmir2::int_div const& div);
    bool exec_instr_val(vmir2::int_mod const& mod);
    bool exec_instr_val(vmir2::store_to_ref const& str);
    bool exec_instr_val(vmir2::load_const_int const& lci);
};

void quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::call_func(type_symbol functype, vmir2::invocation_args args)
{
    stack.emplace_back();
    stack.back().type = functype;
    stack.back().address = {functype, functanoids.at(functype).entry_block, 0};
    // TODO: Args
}
bool quxlang::vmir2::ir2_interpreter::ir2_interpreter_impl::exec_instr()
{
    interp_addr current_instr_address = stack.back().address;

    auto const& current_func = stack.back().type;

    auto const& current_func_ir = functanoids.at(current_func.get());

    auto const& current_block = current_func_ir.blocks.at(current_instr_address.block);

    if (current_instr_address.instruction_index < current_block.instructions.size())
    {
        vm_instruction const& instr = current_block.instructions.at(current_instr_address.instruction_index);

        // If there is an error here, it usually means there is an instruction which is not implemented
        // on the constexpr virtual machine. Instructions which are illegal in a constexpr context
        // should be implemented to throw a derivative of std::logic_error.
        return rpnx::apply_visitor< bool >(
            [this](auto const& param)
            {
                return this->exec_instr_val(param);
            },
            instr);
    }


    auto const terminator_instruction = current_block.terminator;
    if (!terminator_instruction)
    {
        throw std::logic_error("Constexpr execution reached end of block with undefined behavior");
    }

    return rpnx::apply_visitor< bool >(
        [this](auto const& param)
        {
            return this->exec_instr_val(param);
        },
        *terminator_instruction);
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
}
