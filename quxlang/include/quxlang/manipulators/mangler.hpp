// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD
#define QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>


#include "quxlang/data/canonical_resolved_function_chain.hpp"
#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/constexpr_types.hpp>
#include <quxlang/manipulators/expression_stringifier.hpp>

namespace quxlang
{
    std::string mangle_internal(std::string const& str);
    std::string mangle_internal(type_symbol const& qt);
    std::string mangle_internal(argif const& arg);
    std::string mangle_parameter_instantiation(parameter_instantiation const& param);
    std::string mangle_constexpr_value(constexpr_value const& value);

    std::string mangle(type_symbol const& func);

    /**
     * Produces a short stable hash fragment for mangled components that may be too large for filenames.
     */
    inline auto mangle_hash(std::string_view text) -> std::string
    {
        std::uint64_t hash = 14695981039346656037ULL;
        for (char const ch : text)
        {
            hash ^= static_cast< std::uint8_t >(ch);
            hash *= 1099511628211ULL;
        }

        std::string output = "H";
        for (int shift = 60; shift >= 0; shift -= 4)
        {
            output += "0123456789ABCDEF"[(hash >> shift) & 0x0F];
        }
        return output;
    }

    /**
     * Mangles a string with a length prefix so adjacent names cannot collide.
     */
    inline auto mangle_counted_string(std::string const& text) -> std::string
    {
        return std::to_string(text.size()) + "_" + mangle_internal(text);
    }

    /**
     * Mangles an expression by hashing its canonical debug text.
     */
    inline auto mangle_expression(expression const& expr) -> std::string
    {
        return "X" + mangle_hash(to_string(expr));
    }

    /**
     * Mangles a byte payload directly when small and by length plus hash when large.
     */
    inline auto mangle_byte_payload(std::vector< std::byte > const& bytes) -> std::string
    {
        std::string hex;
        hex.reserve(bytes.size() * 2);
        for (std::byte const byte : bytes)
        {
            unsigned const raw = static_cast< unsigned >(byte);
            hex += "0123456789ABCDEF"[raw >> 4];
            hex += "0123456789ABCDEF"[raw & 0x0F];
        }

        if (bytes.size() > 32)
        {
            return "N" + std::to_string(bytes.size()) + mangle_hash(hex);
        }
        return "N" + std::to_string(bytes.size()) + "B" + hex;
    }

    /**
     * Mangles a constexpr antestatal address path.
     */
    inline auto mangle_antestatal_access(antestatal_access const& access) -> std::string
    {
        if (access.template type_is< antestatal_nullptr >())
        {
            return "N";
        }
        if (access.template type_is< antestatal_access_global >())
        {
            return "G" + mangle_internal(access.template get_as< antestatal_access_global >().symbol);
        }
        if (access.template type_is< antestatal_access_field >())
        {
            antestatal_access_field const& field = access.template get_as< antestatal_access_field >();
            return "F" + mangle_antestatal_access(field.object) + mangle_counted_string(field.field_name);
        }
        if (access.template type_is< antestatal_access_array_element >())
        {
            antestatal_access_array_element const& element = access.template get_as< antestatal_access_array_element >();
            return "I" + mangle_antestatal_access(element.array) + std::to_string(element.index) + "E";
        }

        throw quxlang::compiler_bug("unhandled antestatal access kind while mangling");
    }

    /**
     * Mangles a complete antestatal value without size reduction.
     */
    inline auto mangle_antestatal_value_full(antestatal_value const& value) -> std::string
    {
        if (value.template type_is< antestatal_primitive >())
        {
            return "P" + mangle_byte_payload(value.template get_as< antestatal_primitive >().value);
        }
        if (value.template type_is< antestatal_ptrref >())
        {
            return "R" + mangle_antestatal_access(value.template get_as< antestatal_ptrref >().target);
        }
        if (value.template type_is< antestatal_array >())
        {
            std::string output = "A";
            for (antestatal_value const& element : value.template get_as< antestatal_array >().elements)
            {
                output += mangle_antestatal_value_full(element);
            }
            output += "E";
            return output;
        }
        if (value.template type_is< antestatal_struct >())
        {
            std::string output = "S";
            for (auto const& [field_name, field_value] : value.template get_as< antestatal_struct >().fields)
            {
                output += mangle_counted_string(field_name);
                output += mangle_antestatal_value_full(field_value);
            }
            output += "E";
            return output;
        }
        if (value.template type_is< antestatal_interface >())
        {
            antestatal_interface const& interface_value = value.template get_as< antestatal_interface >();
            std::string output = "I";
            output += mangle_internal(interface_value.interface_type);
            output += interface_value.is_default ? "D" : "C";
            for (auto const& [slot, function] : interface_value.functions)
            {
                output += mangle_counted_string(slot.name);
                for (type_symbol const& param : slot.concrete_params.positional)
                {
                    output += "P" + mangle_internal(param);
                }
                for (auto const& [name, param] : slot.concrete_params.named)
                {
                    output += "N" + mangle_counted_string(name) + mangle_internal(param);
                }
                output += "R";
                output += slot.concrete_return_type.has_value() ? mangle_internal(*slot.concrete_return_type) : mangle_internal(type_symbol(void_type{}));
                output += "F" + mangle_internal(function);
            }
            output += "E";
            return output;
        }

        throw quxlang::compiler_bug("unhandled antestatal value kind while mangling");
    }

    /**
     * Mangles an antestatal value, hashing the full spelling once it becomes too large for filenames.
     */
    inline auto mangle_antestatal_value(antestatal_value const& value) -> std::string
    {
        std::string full = mangle_antestatal_value_full(value);
        if (full.size() > 96)
        {
            return "Y" + std::to_string(full.size()) + mangle_hash(full);
        }
        return full;
    }

    /**
     * Mangles a constexpr value for value-template instantiations.
     */
    inline std::string mangle_constexpr_value(constexpr_value const& value)
    {
        if (value.template type_is< antestatal_value >())
        {
            return "A" + mangle_antestatal_value(value.template get_as< antestatal_value >());
        }
        if (value.template type_is< constexpr_serialoid >())
        {
            return "S" + mangle_byte_payload(value.template get_as< constexpr_serialoid >().bytes);
        }
        if (value.template type_is< constexpr_string >())
        {
            return "T" + mangle_byte_payload(value.template get_as< constexpr_string >().bytes);
        }

        throw quxlang::compiler_bug("unhandled constexpr value kind while mangling");
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
        if (arg.requires_static_value)
        {
            result += "S";
        }
        result += mangle_internal(arg.type);

        return result;
    }

    inline std::string mangle_internal(type_symbol const& qt)
    {
        if (qt.template type_is< void_type >())
        {
            return "V";
        }
        else if (qt.template type_is< byte_type >())
        {
            return "B";
        }
        else if (qt.template type_is< initguard_type >())
        {
            return "G";
        }
        else if (qt.template type_is< initguard_lock_type >())
        {
            return "GL";
        }
        else if (qt.template type_is< bool_type >())
        {
            return "Z";
        }
        else if (qt.template type_is< size_type >())
        {
            return "SZ";
        }
        else if (qt.template type_is< thistype >())
        {
            return "TH";
        }
        else if (qt.template type_is< numeric_literal_reference >())
        {
            return "LN";
        }
        else if (qt.template type_is< string_literal_reference >())
        {
            return "LS";
        }
        else if (qt.template type_is< context_reference >())
        {
            return "CX";
        }
        else if (qt.template type_is< freebound_identifier >())
        {
            return "FB" + mangle_counted_string(qt.template get_as< freebound_identifier >().name);
        }
        else if (qt.template type_is< builtin_symbol >())
        {
            return "BI" + mangle_counted_string(qt.template get_as< builtin_symbol >().name);
        }
        else if (qt.template type_is< auto_temploidic >())
        {
            return "TA" + mangle_counted_string(qt.template get_as< auto_temploidic >().name);
        }
        else if (qt.template type_is< decay_temploidic >())
        {
            return "TD" + mangle_counted_string(qt.template get_as< decay_temploidic >().name);
        }
        else if (qt.template type_is< type_temploidic >())
        {
            return "TT" + mangle_counted_string(qt.template get_as< type_temploidic >().name);
        }
        else if (qt.type() == typeid(absolute_module_reference))
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
                throw quxlang::compiler_bug("unknown pointer class while mangling: " + to_string(qt));
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
                throw quxlang::compiler_bug("unknown pointer qualifier while mangling: " + to_string(qt));
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
        else if (qt.template type_is< readonly_constant >())
        {
            switch (qt.template get_as< readonly_constant >().kind)
            {
            case constant_kind::data:
                return "RD";
            case constant_kind::numeric:
                return "RN";
            case constant_kind::string:
                return "RS";
            case constant_kind::cstring:
                return "RC";
            }
            throw quxlang::compiler_bug("unknown readonly constant kind");
        }
        else if (qt.template type_is< nvalue_slot >())
        {
            return "NV" + mangle_internal(qt.template get_as< nvalue_slot >().target);
        }
        else if (qt.template type_is< dvalue_slot >())
        {
            return "DV" + mangle_internal(qt.template get_as< dvalue_slot >().target);
        }
        else if (qt.template type_is< attached_type_reference >())
        {
            attached_type_reference const& ref = qt.template get_as< attached_type_reference >();
            return "AT" + mangle_internal(ref.carrying_type) + "S" + mangle_internal(ref.attached_symbol) + "E";
        }
        else if (qt.template type_is< array_type >())
        {
            array_type const& ref = qt.template get_as< array_type >();
            return "AR" + mangle_expression(ref.element_count) + "T" + mangle_internal(ref.element_type) + "E";
        }
        else if (qt.template type_is< storage >())
        {
            std::string output = "ST";
            for (type_symbol const& storable_type : qt.template get_as< storage >().storable_types)
            {
                output += mangle_internal(storable_type);
            }
            output += "E";
            return output;
        }
        else if (qt.template type_is< aligned_storage >())
        {
            aligned_storage const& ref = qt.template get_as< aligned_storage >();
            return "AS" + mangle_expression(ref.size) + "A" + mangle_expression(ref.align) + "E";
        }
        else if (qt.template type_is< array_initializer_type >())
        {
            array_initializer_type const& ref = qt.template get_as< array_initializer_type >();
            return "AI" + std::to_string(ref.count) + "T" + mangle_internal(ref.element_type) + "E";
        }
        else if (qt.template type_is< static_local_ref >())
        {
            static_local_ref const& ref = qt.template get_as< static_local_ref >();
            return "SL" + mangle_internal(ref.functanoid) + "N" + mangle_counted_string(ref.name) + "G" + std::to_string(ref.generation) + "E";
        }
        else if (qt.template type_is< static_snapshot_ref >())
        {
            static_snapshot_ref const& ref = qt.template get_as< static_snapshot_ref >();
            return "SS" + mangle_internal(ref.functanoid) + "N" + mangle_counted_string(ref.name) + "G" + std::to_string(ref.generation) + "P" + std::to_string(ref.snapshot_id) + "E";
        }
        else if (qt.template type_is< initialization_reference >())
        {
            throw quxlang::compiler_bug("initialization_reference is not a canonical symbol and cannot be mangled; instantiate it first");
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
            pack_arg_type_ref const& ref = qt.template get_as< pack_arg_type_ref >();
            return "PA" + mangle_counted_string(ref.pack_name) + mangle_expression(ref.index) + "E";
        }
        else if (qt.template type_is< value_expression_reference >())
        {
            return "VE";
        }
        else if (qt.template type_is< decltype_type_ref >())
        {
            return "DE" + mangle_internal(qt.template get_as< decltype_type_ref >().symbol) + "E";
        }
        else if (qt.template type_is< typeof_type_ref >())
        {
            return "TO" + mangle_expression(qt.template get_as< typeof_type_ref >().expr) + "E";
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
            if (as< temploid_reference >(qt).which.priority.has_value())
            {
                str += "Y" + std::to_string(*as< temploid_reference >(qt).which.priority);
            }
            if (as< temploid_reference >(qt).which.enable_if.has_value())
            {
                str += "X" + mangle_expression(*as< temploid_reference >(qt).which.enable_if);
            }
            str += "E";

            return str;
        }

        throw quxlang::compiler_bug("unhandled type symbol while mangling: " + to_string(qt));
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
