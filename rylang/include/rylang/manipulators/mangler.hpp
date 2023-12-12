//
// Created by Ryan Nicholl on 11/4/23.
//

#ifndef RYLANG_MANGLER_HEADER_GUARD
#define RYLANG_MANGLER_HEADER_GUARD
#include <string>

#include <boost/type_index.hpp>

#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    inline std::string mangle_internal(type_symbol const& qt)
    {
        if (qt.type() == boost::typeindex::type_id< module_reference >())
        {
            return "M" + (boost::get< module_reference >(qt)).module_name;
        }
        else if (qt.type() == boost::typeindex::type_id< subentity_reference >())
        {
            subentity_reference const& se = boost::get< subentity_reference >(qt);

            return mangle_internal(se.parent) + "N" + se.subentity_name;
        }
        else if (qt.type() == boost::typeindex::type_id< subdotentity_reference >())
        {
            subdotentity_reference const& se = boost::get< subdotentity_reference >(qt);

            return mangle_internal(se.parent) + "D" + se.subdotentity_name;
        }
        else if (qt.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return "P" + mangle_internal(boost::get< instance_pointer_type >(qt).target);
        }
        else if (qt.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
        {
            auto const& i = boost::get< primitive_type_integer_reference >(qt);
            if (i.has_sign)
            {
                return "I" + std::to_string(i.bits);
            }
            else
            {
                return "U" + std::to_string(i.bits);
            }
        }
        else if (qt.type() == boost::typeindex::type_id< instanciation_reference >())
        {
            std::string out = mangle_internal(boost::get< instanciation_reference >(qt).callee);
            out += "C";
            // out += std::to_string(boost::get< functanoid_reference >(qt).parameters.size());
            for (auto const& p : boost::get< instanciation_reference >(qt).parameters)
            {
                out += "A";
                out += mangle_internal(p);
            }
            out += "E";
            return out;
        }
        else if (qt.type() == boost::typeindex::type_id< value_expression_reference >())
        {
            // TODO: Implement
            throw std::runtime_error("unimplemented");
        }
        else if (typeis< mvalue_reference >(qt))
        {
            return "RM" + mangle_internal(as< mvalue_reference >(qt).target);
        }
        else if (typeis< cvalue_reference >(qt))
        {
            return "RC" + mangle_internal(as< cvalue_reference >(qt).target);
        }
        else if (typeis< ovalue_reference >(qt))
        {
            return "RO" + mangle_internal(as< ovalue_reference >(qt).target);
        }
        if (typeis< tvalue_reference >(qt))
        {
            return "RT" + mangle_internal(as< tvalue_reference >(qt).target);
        }

        throw std::runtime_error("unimplemented");
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
} // namespace rylang

#endif // RYLANG_MANGLER_HEADER_GUARD
