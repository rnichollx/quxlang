// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/manipulators/vmmanip.hpp>
#include <quxlang/vmir2/assembly.hpp>

namespace quxlang::vmir2
{
    std::string assembler::to_string(vmir2::functanoid_routine2 fnc)
    {
        std::string output;

        static const std::string indent = "    ";
        output += "[Slots]:\n";

        for (std::size_t i = 1; i < fnc.slots.size(); i++)
        {
            output += indent + "" + std::to_string(i) + ": " + this->to_string(fnc.slots.at(i));
            output += "\n";
        }

        output += "[Blocks]:\n";

        for (std::size_t i = 0; i < fnc.blocks.size(); i++)
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

            output += block_name;

            auto const& blk = fnc.blocks.at(i);

            output += " [< ";
            bool first = true;
            for (auto& [k, v] : blk.entry_state)
            {
                if (v.alive)
                {
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        output += ", ";
                    }
                    output += "%" + std::to_string(k);
                }


            }

            output += " >]";

            output += ":\n";
            output += this->to_string(fnc.blocks.at(i));
            output += "\n";
        }

        return output;
    }
    std::string assembler::to_string(vmir2::executable_block const& inst)
    {
        std::string output;
        static const std::string indent = "    ";
        for (auto& i : inst.instructions)
        {
            output += indent + this->to_string(i);
            output += "\n";
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
        std::string result = "ACF %" + std::to_string(inst.base_index) + ", %" + std::to_string(inst.store_index) + ", " + std::to_string(inst.offset);

        result += " // type1=";
        result += quxlang::to_string(this->m_what.slots.at(inst.base_index).type);
        result += " type2=";
        result += quxlang::to_string(this->m_what.slots.at(inst.store_index).type);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::invoke inst)
    {
        return "IVK " + quxlang::to_string(inst.what) + ", " + this->to_string_internal(inst.args);
    }

    std::string assembler::to_string_internal(vmir2::make_reference inst)
    {
        std::string result = "MKR %" + std::to_string(inst.value_index) + ", %" + std::to_string(inst.reference_index);

        result += " // type1=";
        result += quxlang::to_string(this->m_what.slots.at(inst.value_index).type);

        result += " type2=";

        result += quxlang::to_string(this->m_what.slots.at(inst.reference_index).type);

        return result;
    }

    std::string assembler::to_string_internal(vmir2::cast_reference inst)
    {
        std::string result = "CSR %" + std::to_string(inst.source_ref_index) + ", %" + std::to_string(inst.target_ref_index) + ", " + std::to_string(inst.offset);

        result += " // type1=";

        result += quxlang::to_string(this->m_what.slots.at(inst.source_ref_index).type);
        result += " type2=";
        result += quxlang::to_string(this->m_what.slots.at(inst.target_ref_index).type);

        return result;
    }
    std::string assembler::to_string_internal(vmir2::constexpr_set_result inst)
    {
        std::string result = "CSR %" + std::to_string(inst.target);
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
        std::string output = "LCV %" + std::to_string(inst.target) + ", {";

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
    std::string assembler::to_string_internal(vmir2::load_const_int inst)
    {
        return "LCI %" + std::to_string(inst.target) + ", " + inst.value;
    }

    std::string assembler::to_string_internal(vmir2::make_pointer_to inst)
    {
        return "MPT %" + std::to_string(inst.of_index) + ", %" + std::to_string(inst.pointer_index);
    }
    std::string assembler::to_string_internal(vmir2::load_from_ref inst)
    {
        return "LFR %" + std::to_string(inst.from_reference) + ", %" + std::to_string(inst.to_value);
    }
    std::string assembler::to_string_internal(vmir2::store_to_ref inst)
    {
        return "STR %" + std::to_string(inst.from_value) + ", %" + std::to_string(inst.to_reference);
    }
    std::string assembler::to_string_internal(vmir2::dereference_pointer inst)
    {
        return "DRP %" + std::to_string(inst.from_pointer) + ", %" + std::to_string(inst.to_reference);
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
        return "LCZ %" + std::to_string(inst.target);
    }

} // namespace quxlang::vmir2
