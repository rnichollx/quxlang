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
        output += "slots:";

        for (auto& i : inst.slots)
        {
            output += this->to_string(i);
        }

        output += "instructions:";

        for (auto& i : inst.instructions)
        {
            output += this->to_string(i);
        }

        return output;
    }
    std::string assembler::to_string(vmir2::vm_instruction inst)
    {
        return rpnx::apply_visitor< std::string >(
            [&](auto&& x)
            {
                return to_string(x);
            },
            inst);
    }
    std::string assembler::to_string(vmir2::vm_slot slt)
    {
        return "SLOT " + quxlang::to_string(slt.type);
    }

    std::string assembler::to_string_internal(vmir2::access_field inst)
    {
        return "ACF " + std::to_string(inst.base_index) + ", " + std::to_string(inst.store_index) + ", " + std::to_string(inst.offset);
    }
    std::string assembler::to_string_internal(vmir2::invoke inst)
    {
        return "IVK " + quxlang::to_string(inst.what) + ", " + this->to_string_internal(inst.args);
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
