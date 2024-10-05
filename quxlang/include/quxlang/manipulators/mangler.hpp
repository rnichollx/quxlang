// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD
#define QUXLANG_MANIPULATORS_MANGLER_HEADER_GUARD
#include <string>

#include <boost/type_index.hpp>

#include "quxlang/data/canonical_resolved_function_chain.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
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
    inline std::string mangle_internal(type_symbol const& qt)
    {
        if (qt.type() == typeid(module_reference))
        {
            return "M" + (as< module_reference >(qt)).module_name;
        }
        else if (qt.type() == boost::typeindex::type_id< subsymbol >())
        {
            subsymbol const& se = as< subsymbol >(qt);

            return mangle_internal(se.of) + "N" + se.name;
        }
        else if (qt.type() == boost::typeindex::type_id< submember >())
        {
            submember const& se = as< submember >(qt);

            return mangle_internal(se.of) + "D" + mangle_internal(se.name);
        }
        else if (qt.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return "P" + mangle_internal(as< instance_pointer_type >(qt).target);
        }
        else if (qt.type() == boost::typeindex::type_id< int_type >())
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
        else if (qt.type() == boost::typeindex::type_id< instantiation_type >())
        {
            std::string out = mangle_internal(as< instantiation_type >(qt).callee);
            out += "C";
            // out += std::to_string(as< functanoid_reference >(qt).parameters.size());
            for (auto const& p : as< instantiation_type >(qt).parameters.positional)
            {
                out += "AP";
                out += mangle_internal(p);
            }
            for (auto const& p : as< instantiation_type >(qt).parameters.named)
            {
                out += "AN";
                out += p.first;
                out += "A";
                out += mangle_internal(p.second);
            }
            out += "E";
            return out;
        }
        else if (qt.type() == boost::typeindex::type_id< value_expression_reference >())
        {
            // TODO: Implement
            throw std::logic_error("unimplemented");
        }
        else if (typeis< mvalue_reference >(qt))
        {
            return "RM" + mangle_internal(as< mvalue_reference >(qt).target);
        }
        else if (typeis< cvalue_reference >(qt))
        {
            return "RC" + mangle_internal(as< cvalue_reference >(qt).target);
        }
        else if (typeis< wvalue_reference >(qt))
        {
            return "RO" + mangle_internal(as< wvalue_reference >(qt).target);
        }
        else if (typeis< tvalue_reference >(qt))
        {
            return "RT" + mangle_internal(as< tvalue_reference >(qt).target);
        }
        else if (typeis< selection_reference >(qt))
        {

            std::string str;

            str += mangle_internal(as< selection_reference >(qt).templexoid);
            str += "S";

            for (auto const& p : as< selection_reference >(qt).overload.call_parameters.positional)
            {
                str += "AP";
                str += mangle_internal(p);
            }
            for (auto const& p : as< selection_reference >(qt).overload.call_parameters.named)
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
