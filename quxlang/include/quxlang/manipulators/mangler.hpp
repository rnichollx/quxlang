// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD
#define QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD
#include <string>


#include "quxlang/data/canonical_resolved_function_chain.hpp"
#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/constexpr_types.hpp>

namespace quxlang
{
    std::string mangle_internal(std::string const& str);
    std::string mangle_internal(type_symbol const& qt);
    std::string mangle_internal(argif const& arg);
    std::string mangle_parameter_instantiation(parameter_instantiation const& param);

    std::string mangle(type_symbol const& func);

    inline std::string mangle_constexpr_value(constexpr_value const& value)
    {
        auto const& antestatal = constexpr_value_as_antestatal(value);
        if (typeis< antestatal_primitive >(antestatal))
        {
            std::string output = "P";
            for (auto byte : as< antestatal_primitive >(antestatal).value)
            {
                auto raw = static_cast< unsigned >(byte);
                output += "0123456789ABCDEF"[raw >> 4];
                output += "0123456789ABCDEF"[raw & 0x0F];
            }
            return output;
        }
        return "X";
    }

    inline std::string mangle_parameter_instantiation(parameter_instantiation const& param)
    {
        if (param.template type_is< parameter_type_instantiation >())
        {
            return mangle_internal(param.template get_as< parameter_type_instantiation >().type);
        }

        auto const& value = param.template get_as< parameter_value_instantiation >();
        return "V" + mangle_internal(value.type) + mangle_constexpr_value(value.value);
    }

    inline std::string mangle_internal(std::string const& str)
    {
        std::string result;
        bool upper = false;
        for (char c : str)
        {
            if (c >= 'A' && c <= 'Z')
            {
                upper = true;
                result += c - 'A' + 'a';
            }

            else
            {
                result += c;
            }
        }

        if (upper)
        {
            return "W" + result;
        }
        else
        {
            return result;
        }
    }

    inline std::string mangle_internal(argif const& arg)
    {
        std::string result;
        if (arg.is_defaulted)
        {
            result += "D";
        }
        if (arg.is_pack)
        {
            result += "V";
        }
        result += mangle_internal(arg.type);

        return result;
    }

    inline std::string mangle_internal(type_symbol const& qt)
    {
        if (qt.type() == typeid(absolute_module_reference))
        {
            return "M" + (as< absolute_module_reference >(qt)).module_name;
        }
        else if (qt.template type_is< subsymbol >())
        {
            subsymbol const& se = as< subsymbol >(qt);

            return mangle_internal(se.of) + "N" + se.name;
        }
        else if (qt.template type_is< submember >())
        {
            submember const& se = as< submember >(qt);

            return mangle_internal(se.of) + "D" + mangle_internal(se.name);
        }
        else if (qt.template type_is< ptrref_type >())
        {
            std::string output = "P";

            auto const& ptr = as< ptrref_type >(qt);

            switch (ptr.ptr_class)
            {
            case pointer_class::instance:
                output += "I";
                break;
            case pointer_class::array:
                output += "A";
                break;
            case pointer_class::machine:
                output += "M";
                break;
            case pointer_class::ref:
                output += "R";
                break;
            default:
                throw std::logic_error("unimplemented");
            }
            switch (ptr.qual)
            {
            case qualifier::constant:
                output += "C";
                break;
            case qualifier::mut:
                output += "M";
                break;
            case qualifier::temp:
                output += "T";
                break;
            case qualifier::write:
                output += "W";
                break;
            case qualifier::input:
                output += "I";
                break;
            case qualifier::output:
                output += "O";
                break;
            case qualifier::auto_:
                output += "A";
                break;
            default:
                throw std::logic_error("unimplemented");
            }
            output += mangle_internal(ptr.target);
            return output;
        }
        else if (qt.template type_is< constexpr_proxy >())
        {
            return "KXP";
        }
        else if (qt.template type_is< procedure_type >())
        {
            auto const& proc = as< procedure_type >(qt);
            std::string output = "Q";
            auto mangled_cc = mangle_internal(proc.calling_convention);
            output += std::to_string(mangled_cc.size());
            output += mangled_cc;
            output += proc.is_noexcept ? "N" : "X";

            for (auto const& [name, arg] : proc.signature.params.named)
            {
                output += "AN";
                output += name;
                output += "A";
                output += mangle_internal(arg);
            }
            for (auto const& arg : proc.signature.params.positional)
            {
                output += "AP";
                output += mangle_internal(arg);
            }

            output += "R";
            output += mangle_internal(proc.signature.return_type.value_or(type_symbol(void_type{})));
            output += "E";
            return output;
        }
        else if (qt.template type_is< int_type >())
        {
            auto const& i = as< int_type >(qt);
            if (i.has_sign)
            {
                return "I" + std::to_string(i.bits);
            }
            else
            {
                return "U" + std::to_string(i.bits);
            }
        }
        else if (qt.template type_is< float_type >())
        {
            auto const& f = as< float_type >(qt);
            return "F" + std::to_string(f.bits) + "E" + std::to_string(f.exponent_bits);
        }
        else if (qt.template type_is< initialization_reference >())
        {
            std::string out = mangle_internal(as< initialization_reference >(qt).initializee);
            out += "C";
            // out += std::to_string(as< functanoid_reference >(qt).parameters.size());
            for (auto const& p : as< initialization_reference >(qt).parameters.positional)
            {
                out += "AP";
                out += mangle_parameter_instantiation(p);
            }
            for (auto const& p : as< initialization_reference >(qt).parameters.named)
            {
                out += "AN";
                out += p.first;
                out += "A";
                out += mangle_parameter_instantiation(p.second);
            }
            out += "E";
            return out;
        }
        else if (qt.template type_is< instanciation_reference >())
        {
            auto const& inst = as< instanciation_reference >(qt);
            std::string out = mangle_internal(type_symbol(inst.temploid));
            out += "I";
            for (auto const& p : inst.params.positional)
            {
                out += "AP";
                out += mangle_parameter_instantiation(p);
            }
            for (auto const& p : inst.params.named)
            {
                out += "AN";
                out += p.first;
                out += "A";
                out += mangle_parameter_instantiation(p.second);
            }
            out += "E";
            return out;
        }
        else if (qt.template type_is< pack_arg_type_ref >())
        {
            throw std::logic_error("PACK_ARG_TYPE must be resolved before mangling");
        }
        else if (qt.template type_is< value_expression_reference >())
        {
            // TODO: Implement
            throw std::logic_error("unimplemented");
        }
        else if (typeis< temploid_reference >(qt))
        {

            std::string str;

            str += mangle_internal(as< temploid_reference >(qt).templexoid);
            str += "S";

            for (auto const& p : as< temploid_reference >(qt).which.interface.positional)
            {
                str += "AP";
                str += mangle_internal(p);
            }
            for (auto const& p : as< temploid_reference >(qt).which.interface.named)
            {
                str += "AN";
                str += p.first;
                str += "A";
                str += mangle_internal(p.second);
            }
            str += "E";
            // TODO: Named parameters here

            return str;
        }

        throw std::logic_error("unimplemented");
    }

    inline std::string mangle_internal [[deprecated("qualified")]] (canonical_resolved_function_chain const& func)
    {
        std::string out = mangle_internal(func.function_entity_chain);
        out += "F";
        out += std::to_string(func.overload_index);
        return out;
    }

    inline std::string mangle [[deprecated("qualified")]] (canonical_resolved_function_chain const& func)
    {
        std::string out = "_S_";
        out += mangle_internal(func);
        return out;
    }

    inline std::string mangle(type_symbol const& func)
    {
        std::string out = "_S_";
        out += mangle_internal(func);
        return out;
    }
} // namespace quxlang

#endif // QUXLANG_MANGLER_HEADER_GUARD
