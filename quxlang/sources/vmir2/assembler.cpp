// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/expr/co_vmir_codegen_emitter.hpp"
#include <quxlang/vmir2/assembly.hpp>

namespace quxlang::vmir2
{
    std::string assembler::to_string(vmir2::functanoid_routine3 fnc)
    {
        std::string output;

        static const std::string indent = "    ";
        output += "[DTors]:\n";

        for (auto const& [type, dtor] : fnc.non_trivial_dtors)
        {
            output += indent + quxlang::to_string(type) + " USES " + quxlang::to_string(dtor) + "\n";
        }

        output += "[Slots]:\n";

        for (std::size_t i = 0; i < fnc.local_types.size(); i++)
        {
            output += indent + std::to_string(i) + ": " + this->to_string(fnc.local_types.at(i));
            output += "\n";
        }

        output += "[Parameters]:\n";
        output += indent + quxlang::to_string(fnc.parameters) + "\n";

        output += "[Blocks]:\n";

        for (block_index i = block_index(0); i < fnc.blocks.size(); i++)
        {
            std::string block_name;
            if (fnc.block_names.contains(i))
            {
                block_name = "BLOCK" + std::to_string(i) + "[" + fnc.block_names.at(i) + "]";
            }
            else
            {
                block_name = "BLOCK" + std::to_string(i);
            }

            output += block_name + " " + to_string(fnc.blocks.at(i).entry_state);
            if (fnc.blocks.at(i).dbg_name.has_value())
            {
                output += " // " + fnc.blocks.at(i).dbg_name.value();
            }
            output += "\n";
            output += this->to_string(fnc.blocks.at(i));
            output += "\n";
        }

        return output;
    }
    std::string assembler::to_string(vmir2::executable_block const& inst)
    {

        state = inst.entry_state;

        std::string output;
        static const std::string indent = "    ";
        for (auto& i : inst.instructions)
        {
            output += indent + this->to_string(i);
            output += "\n";

            try
            {
                auto const& what = m_what;
                codegen_state_engine(this->state, what.local_types, {}).apply(i);
                if (print_states)
                {
                    output += indent + "// state: " + this->to_string(this->state) + "\n";
                }
            }
            catch (std::exception const& e)
            {
                if (print_states)
                {
                    output += indent + "// state: exception: " + e.what() + "\n";
                }

            }
        }
        if (!inst.terminator.has_value())
        {
            output += indent + "MISSING_TERMINATOR\n";
        }
        else
        {
            output += indent + this->to_string(inst.terminator.value()) + "\n";
        }
        return output;
    }
    std::string assembler::to_string(vmir2::state_map const& state)
    {
        std::string output;
        output += " [< ";
        bool first = true;
        for (auto& [k, v] : state)
        {
            output += this->to_string(k, v);
        }

        output += " >]";
        return output;
    }
    std::string assembler::to_string(std::size_t index, vmir2::slot_state const& v)
    {
        std::string output;
        bool first = true;
        if (v.alive() || v.storage_valid)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += "%" + std::to_string(index);

            if (v.stage == slot_stage::full)
            {
                output += "[A]";
            }
            else if (v.stage == slot_stage::partial)
            {
                output += "[P]";
            }
            else if (v.storage_valid)
            {
                output += "[S]";
            }

            if (v.delegate_of)
            {
                output += "[D <- %" + std::to_string(*v.delegate_of) + "]";
            }

            if (v.array_delegate_of_initializer)
            {
                output += "[D <== %" + std::to_string(*v.array_delegate_of_initializer) + "]";
            }
            if (v.delegates.has_value())
            {
                auto const& d = v.delegates.value();
                for (auto const& [name, idx] : d.named)
                {
                    output += "[D @" + name + " -> %" + std::to_string(idx) + "]";
                }
                std::size_t i = 0;
                for (auto const& idx : d.positional)
                {

                    output += "[D %["+std::to_string(i++) + "] -> %" + std::to_string(idx) + "]";
                }
            }
        }
        return output;
    }
    std::string assembler::to_string(vmir2::vm_instruction inst)
    {
        return rpnx::apply_visitor< std::string >(
            [&](auto&& x)
            {
                return this->to_string_internal(x);
            },
            inst);
    }

    std::string assembler::to_string(vmir2::vm_terminator inst)
    {
        return rpnx::apply_visitor< std::string >(
            [&](auto&& x)
            {
                return this->to_string_internal(x);
            },
            inst);
    }

    std::string assembler::to_string(vmir2::local_type lct)
    {
        return quxlang::to_string(lct.type);
    }

    std::string assembler::to_string(vmir2::vm_slot slt)
    {
        std::string output;

        switch (slt.kind)
        {
        case vmir2::slot_kind::local:
            output = "LOCAL";
            break;
        case vmir2::slot_kind::positional_arg:
        case vmir2::slot_kind::named_arg:
            output = "ARG";
            break;
        case vmir2::slot_kind::binding:
            output = "BINDING";
            break;
        case vmir2::slot_kind::literal:
            output = "LITERAL";
            break;
        default:
            throw std::logic_error("Invalid slot kind");
        }

        output += " " + quxlang::to_string(slt.type);

        if (slt.literal_value)
        {
            output += " " + *slt.literal_value;
        }

        if (slt.binding_of)
        {
            output += " BINDS %" + std::to_string(*slt.binding_of);
        }

        if (slt.name)
        {
            output += " // " + *slt.name;
        }

        return output;
    }

    std::string assembler::to_string_internal(vmir2::access_field inst)
    {
        std::string result = "ACCESS_FIELD %" + std::to_string(inst.base_index) + ", %" + std::to_string(inst.store_index) + ", " + inst.field_name;
        // Use apply_visitor since both types have the same logic

        result += " // type1=";
        result += quxlang::to_string(m_what.local_types.at(inst.base_index).type);
        result += " type2=";
        result += quxlang::to_string(m_what.local_types.at(inst.store_index).type);

        return result;
    }

    std::string assembler::to_string_internal(vmir2::decrement inst)
    {
        return "DEC %" + std::to_string(inst.value) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::preincrement inst)
    {
        return "PREINC %" + std::to_string(inst.target) + ", %" + std::to_string(inst.target2);
    }

    std::string assembler::to_string_internal(vmir2::predecrement inst)
    {
        return "PREDEC %" + std::to_string(inst.target) + ", %" + std::to_string(inst.target2);
    }

    std::string assembler::to_string_internal(vmir2::assert_instr const& asrt)
    {
        std::string message;
        // TODO: Escape the message so it wont look weird if it has quotes in it
        message = "\"" + asrt.message + "\"";

        // TODO: convert the location to string also
        return "ASSERT %" + std::to_string(asrt.condition) + ", " + message + ", @@ <TODO>";
    }
    std::string assembler::to_string_internal(vmir2::increment inst)
    {
        return "INC %" + std::to_string(inst.value) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::to_bool inst)
    {
        std::string result = "TOBOOL %" + std::to_string(inst.from) + ", %" + std::to_string(inst.to);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::unimplemented unimpl)
    {
        std::string result = "UNIMPLEMENTED";
        if (unimpl.message.has_value())
        {
            result += " // " + unimpl.message.value();
        }
        return result;
    }



    std::string assembler::to_string_internal(vmir2::to_bool_not inst)
    {
        std::string result = "TBN %" + std::to_string(inst.from) + ", %" + std::to_string(inst.to);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::runtime_ce inst)
    {
        return std::string("RT_CE -> %") + std::to_string(inst.target);
    }

    std::string assembler::to_string_internal(vmir2::array_init_start ani)
    {
        return std::string("ARRAY_INIT_START %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.on_value);
    }

    std::string assembler::to_string_internal(vmir2::array_init_index ani)
    {
        return std::string("ARRAY_INIT_INDEX %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.result);
    }


    std::string assembler::to_string_internal(vmir2::array_init_more ani)
    {
        return std::string("ARRAY_INIT_MORE %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.result);
    }

    std::string assembler::to_string_internal(vmir2::array_init_element ani)
    {
        return std::string("ARRAY_INIT_ELEMENT %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.target);
    }

    std::string assembler::to_string_internal(vmir2::array_init_finish ani)
    {
        return std::string("ARRAY_INIT_FINISH %") + std::to_string(ani.initializer);
    }



    std::string assembler::to_string_internal(vmir2::access_array inst)
    {
        std::string result;
        result += "ACCESS_ARRAY %" + std::to_string(inst.base_index) + ", %" + std::to_string(inst.index_index) + ", %" + std::to_string(inst.store_index);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::invoke inst)
    {
        return "INVOKE " + quxlang::to_string(inst.what) + ", " + this->to_string_internal(inst.args);
    }

    std::string assembler::to_string_internal(vmir2::make_reference inst)
    {
        std::string result = "MAKEREF %" + std::to_string(inst.value_index) + ", %" + std::to_string(inst.reference_index);

        // Use apply_visitor since both types have the same logic

        result += " // type1=";
        result += quxlang::to_string(m_what.local_types.at(inst.value_index).type);
        result += " type2=";
        result += quxlang::to_string(m_what.local_types.at(inst.reference_index).type);

        return result;
    }

    std::string assembler::to_string_internal(vmir2::cast_reference inst)
    {
        std::string result = "CONVERT_REF %" + std::to_string(inst.source_ref_index) + ", %" + std::to_string(inst.target_ref_index);

        // Use apply_visitor since both types have the same logic

        result += " // type1=";
        result += quxlang::to_string(m_what.local_types.at(inst.source_ref_index).type);
        result += " type2=";
        result += quxlang::to_string(m_what.local_types.at(inst.target_ref_index).type);

        return result;
    }
    std::string assembler::to_string_internal(vmir2::copy_reference cpr)
    {
        std::string result = "COPYREF %" + std::to_string(cpr.from_index) + ", %" + std::to_string(cpr.to_index);
        return result;
    }
    std::string assembler::to_string_internal(vmir2::constexpr_set_result inst)
    {
        std::string result = "CE_SETRESULT %" + std::to_string(inst.target);
        return result;
    }
    std::string assembler::to_string_internal(vmir2::jump inst)
    {
        std::string result;
        result += "JUMP !" + std::to_string(inst.target);
        return result;
    }
    std::string assembler::to_string_internal(vmir2::branch inst)
    {
        std::string result;
        result += "BRANCH %" + std::to_string(inst.condition) + ", !" + std::to_string(inst.target_true) + ", !" + std::to_string(inst.target_false);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::ret inst)
    {
        return "RET";
    }

    std::string assembler::to_string_internal(vmir2::invocation_args inst)
    {
        std::string output = "[";
        bool first = true;
        for (auto& i : inst.named)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;

            output += i.first + "=" + std::to_string(i.second);
        }
        for (auto& i : inst.positional)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;

            output += std::to_string(i);
        }

        output += "]";
        return output;
    }
    std::string assembler::to_string_internal(vmir2::load_const_value inst)
    {
        std::string output = "INITVAL %" + std::to_string(inst.target) + ", {";

        bool first = true;
        for (auto& i : inst.value)
        {
            if (!first)
            {
                output += ", ";
            }

            first = false;

            int8_t val = static_cast< uint8_t >(i);

            uint8_t lower = val & 0x0F;
            uint8_t upper = (val & 0xF0) >> 4;

            if (upper < 10)
            {
                output += std::to_string(upper);
            }
            else
            {
                output += 'A' + upper - 10;
            }

            if (lower < 10)
            {
                output += std::to_string(lower);
            }
            else
            {
                output += 'A' + lower - 10;
            }
        }

        output += "}";
        return output;
    }
    std::string assembler::to_string_internal(vmir2::load_const_bool inst)
    {
        std::string result = "INITBOOL %" + std::to_string(inst.target) + ", ";
        if (inst.value)
        {
            result += "true";
        }
        else
        {
            result += "false";
        }
        return result;
    }
    std::string assembler::to_string_internal(vmir2::load_const_int inst)
    {
        return "INIT_INT %" + std::to_string(inst.target) + ", " + inst.value;
    }

    std::string assembler::to_string_internal(vmir2::make_pointer_to inst)
    {
        return "MAKE_PTR %" + std::to_string(inst.of_index) + ", %" + std::to_string(inst.pointer_index);
    }
    std::string assembler::to_string_internal(vmir2::swap swp)
    {
        return "SWAP %" + std::to_string(swp.a) + ", %" + std::to_string(swp.b);
    }
    std::string assembler::to_string_internal(vmir2::load_from_ref inst)
    {
        return "LOAD %" + std::to_string(inst.from_reference) + ", %" + std::to_string(inst.to_value);
    }
    std::string assembler::to_string_internal(vmir2::store_to_ref inst)
    {
        return "STORE %" + std::to_string(inst.from_value) + ", %" + std::to_string(inst.to_reference);
    }
    std::string assembler::to_string_internal(vmir2::dereference_pointer inst)
    {
        return "DEREF %" + std::to_string(inst.from_pointer) + ", %" + std::to_string(inst.to_reference);
    }
    std::string assembler::to_string_internal(vmir2::int_add add)
    {
        return "IADD %" + std::to_string(add.a) + ", %" + std::to_string(add.b) + ", %" + std::to_string(add.result);
    }

    std::string assembler::to_string_internal(vmir2::int_sub sub)
    {
        return "ISUB %" + std::to_string(sub.a) + ", %" + std::to_string(sub.b) + ", %" + std::to_string(sub.result);
    }

    std::string assembler::to_string_internal(vmir2::int_mul mul)
    {
        return "IMUL %" + std::to_string(mul.a) + ", %" + std::to_string(mul.b) + ", %" + std::to_string(mul.result);
    }

    std::string assembler::to_string_internal(vmir2::int_div div)
    {
        return "IDIV %" + std::to_string(div.a) + ", %" + std::to_string(div.b) + ", %" + std::to_string(div.result);
    }

    std::string assembler::to_string_internal(vmir2::int_mod mod)
    {
        return "IMOD %" + std::to_string(mod.a) + ", %" + std::to_string(mod.b) + ", %" + std::to_string(mod.result);
    }
    std::string assembler::to_string_internal(vmir2::load_const_zero inst)
    {
        return "INIT_ZERO %" + std::to_string(inst.target);
    }
    std::string assembler::to_string_internal(vmir2::cmp_eq inst)
    {
        return "CMP_EQ %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::cmp_ne inst)
    {
        return "CMP_NE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::cmp_lt inst)
    {
        return "CMP_LT %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::cmp_ge inst)
    {
        return "CMP_GE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }

    // Pointer compare instructions
    std::string assembler::to_string_internal(vmir2::pcmp_eq inst)
    {
        return "PTR_CMP_EQ %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::pcmp_ne inst)
    {
        return "PTR_CMP_NE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::pcmp_lt inst)
    {
        return "PTR_CMP_LT %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::pcmp_ge inst)
    {
        return "PTR_CMP_GE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }


    std::string assembler::to_string_internal(vmir2::gcmp_eq inst)
    {
        return "GLB_CMP_EQ %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::gcmp_ne inst)
    {
        return "GLB_CMP_NE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::gcmp_lt inst)
    {
        return "GLB_CMP_LT %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::gcmp_ge inst)
    {
        return "GLB_CMP_GE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::defer_nontrivial_dtor dntd)
    {
        return "DEFER_DTOR " + quxlang::to_string(dntd.func) + ", %" + std::to_string(dntd.on_value) + ", %" + this->to_string_internal(dntd.args);
    }
    std::string assembler::to_string_internal(vmir2::struct_init_start sdn)
    {
        return "STRUCT_INIT_START %" + std::to_string(sdn.on_value) + ", " + this->to_string_internal(sdn.fields);
    }
    std::string assembler::to_string_internal(vmir2::struct_init_finish scn)
    {
        return "STRUCT_INIT_FINISH %" + std::to_string(scn.on_value);
    }
    std::string assembler::to_string_internal(vmir2::end_lifetime elt)
    {
        return "END_LIFETIME %" + std::to_string(elt.of);
    }
    std::string assembler::to_string_internal(vmir2::pointer_arith inst)
    {
        return "PTR_ARITH %" + std::to_string(inst.from) + ", " + std::to_string(inst.multiplier) + " %" + std::to_string(inst.offset) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::pointer_diff inst)
    {
        std::string result = "PTR_DIFF %" + std::to_string(inst.from) + ", %" + std::to_string(inst.to) + ", %" + std::to_string(inst.result);
        return result;
    }

} // namespace quxlang::vmir2
