//
// Created by Ryan Nicholl on 11/4/23.
//

#ifndef RPNX_RYANSCRIPT1031_MANGLER_HEADER
#define RPNX_RYANSCRIPT1031_MANGLER_HEADER
#include <string>

#include <boost/type_index.hpp>

#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    inline std::string mangle_internal(qualified_symbol_reference const& qt)
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
        else if (qt.type() == boost::typeindex::type_id< pointer_to_reference >())
        {
            return "P" + mangle_internal(boost::get< pointer_to_reference >(qt).target);
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
        else if (qt.type() == boost::typeindex::type_id< parameter_set_reference >())
        {
            std::string out = mangle_internal(boost::get< parameter_set_reference >(qt).callee);
            out += "C";
            // out += std::to_string(boost::get< parameter_set_reference >(qt).parameters.size());
            for (auto const& p : boost::get< parameter_set_reference >(qt).parameters)
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

        throw std::runtime_error("unimplemented");
    }

    inline std::string mangle_internal(canonical_type_reference const& type)
    {
        std::string out = "";

        if (type.type() == (boost::typeindex::type_id< canonical_pointer_type_reference >()))
        {
            out += "P";
            out += mangle_internal(boost::get< canonical_pointer_type_reference >(type).to);
            return out;
        }
        else if (type.type() == boost::typeindex::type_id< integral_keyword_ast >())
        {
            auto const& kw = boost::get< integral_keyword_ast >(type);
            if (kw.is_signed)
            {
                out += "I";
            }
            else
            {
                out += "U";
            }
            out += std::to_string(kw.size);
            return out;
        }
        else if (type.type() == boost::typeindex::type_id< canonical_lookup_chain >())
        {
            auto const& chain = boost::get< canonical_lookup_chain >(type);
            bool start = true;
            for (std::string_view const s : chain)
            {
                if (!start)
                {
                    out += "N";
                }
                else
                {
                    start = false;
                }
                out += s;
            }

            return out;
        }

        return out;
    }

    inline std::string mangle_internal(canonical_resolved_function_chain const& func)
    {
        std::string out = mangle_internal(func.function_entity_chain);
        out += "F";
        out += std::to_string(func.overload_index);
        return out;
    }

    inline std::string mangle(canonical_type_reference const& type)
    {
        std::string out = "_S_";
        out += mangle_internal(type);
        return out;
    }

    inline std::string mangle(canonical_resolved_function_chain const& func)
    {
        std::string out = "_S_";
        out += mangle_internal(func);
        return out;
    }

    inline std::string mangle(qualified_symbol_reference const& func)
    {
        std::string out = "_S_";
        out += mangle_internal(func);
        return out;
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MANGLER_HEADER
