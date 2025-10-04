// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD
#define QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD
#include <string>


#include "quxlang/data/canonical_resolved_function_chain.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    std::string mangle_internal(std::string const& str);
    std::string mangle_internal(type_symbol const& qt);
    std::string mangle_internal(argif const& arg);

    std::string mangle(type_symbol const& func);

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
        else if (qt.template type_is< initialization_reference >())
        {
            std::string out = mangle_internal(as< initialization_reference >(qt).initializee);
            out += "C";
            // out += std::to_string(as< functanoid_reference >(qt).parameters.size());
            for (auto const& p : as< initialization_reference >(qt).parameters.positional)
            {
                out += "AP";
                out += mangle_internal(p);
            }
            for (auto const& p : as< initialization_reference >(qt).parameters.named)
            {
                out += "AN";
                out += p.first;
                out += "A";
                out += mangle_internal(p.second);
            }
            out += "E";
            return out;
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
