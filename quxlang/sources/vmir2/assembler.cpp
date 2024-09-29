//
// Created by Ryan Nicholl on 6/12/24.
//
#include <quxlang/manipulators/vmmanip.hpp>
#include <quxlang/vmir2/assembly.hpp>

namespace quxlang::vmir2
{
    std::string assembler::to_string(vmir2::functanoid_routine inst)
    {
        std::string output;

        static const std::string indent = "    ";
        output += "slots:\n";

        for (std::size_t i = 1; i < inst.slots.size(); i++)
        {

            output += indent + "" + std::to_string(i) + ": " + this->to_string(inst.slots.at(i));
            output += "\n";
        }

        output += "instructions:\n";

        for (auto& i : inst.instructions)
        {
            output += indent + this->to_string(i);
            output += "\n";
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

} // namespace quxlang::vmir2
