//
// Created by Ryan Nicholl on 11/14/23.
//

#include "rylang/manipulators/qmanip.hpp"

namespace rylang
{
    std::string to_string(call_parameter_information const& ref)
    {
        std::string result = "call_os(";
        for (std::size_t i = 0; i < ref.argument_types.size(); i++)
        {
            if (i != 0)
                result += ", ";
            result += to_string(ref.argument_types[i]);
        }
        result += ")";

        return result;
    }

    std::optional< qualified_symbol_reference > qualified_parent(qualified_symbol_reference input)
    {
        if (input.type() == boost::typeindex::type_id< subentity_reference >())
        {
            return boost::get< subentity_reference >(input).parent;
        }
        else if (input.type() == boost::typeindex::type_id< pointer_to_reference >())
        {
            return boost::get< pointer_to_reference >(input).target;
        }
        else if (input.type() == boost::typeindex::type_id< functanoid_reference >())
        {
            return boost::get< functanoid_reference >(input).callee;
        }
        else
        {
            return std::nullopt;
        }
    }

    qualified_symbol_reference with_context(qualified_symbol_reference const& ref, qualified_symbol_reference const& context)
    {
        if (ref.type() == boost::typeindex::type_id< context_reference >())
        {
            return context;
        }
        else if (ref.type() == boost::typeindex::type_id< subentity_reference >())
        {
            return subentity_reference{with_context(boost::get< subentity_reference >(ref).parent, context), boost::get< subentity_reference >(ref).subentity_name};
        }
        else if (ref.type() == boost::typeindex::type_id< pointer_to_reference >())
        {
            return pointer_to_reference{with_context(boost::get< pointer_to_reference >(ref).target, context)};
        }
        else if (ref.type() == boost::typeindex::type_id< functanoid_reference >())
        {
            functanoid_reference output = boost::get< functanoid_reference >(ref);
            output.callee = with_context(output.callee, context);
            for (auto& p : output.parameters)
            {
                p = with_context(p, context);
            }
            return output;
        }
        else
        {
            return ref;
        }
    }

    std::string qualified_symbol_stringifier::operator()(subentity_reference const& ref) const
    {
        return to_string(ref.parent) + "::" + ref.subentity_name;
    }
    std::string qualified_symbol_stringifier::operator()(pointer_to_reference const& ref) const
    {
        return "->" + boost::apply_visitor(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(functanoid_reference const& ref) const
    {
        std::string output = boost::apply_visitor(*this, ref.callee) + "(";
        bool first = true;
        for (auto& p : ref.parameters)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += boost::apply_visitor(*this, p);
        }
        output += ")";
        return output;
    }
    std::string qualified_symbol_stringifier::operator()(mvalue_reference const& ref) const
    {
        return "MUT& " + boost::apply_visitor(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(tvalue_reference const& ref) const
    {
        return "TEMP& " + boost::apply_visitor(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(cvalue_reference const& ref) const
    {
        return "CONST& " + boost::apply_visitor(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(ovalue_reference const& ref) const
    {
        return "OUT&" + boost::apply_visitor(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(context_reference const& ref) const
    {
        return "context";
    }
    std::string qualified_symbol_stringifier::operator()(bound_function_type_reference const& ref) const
    {
        return "BINDING(" + to_string(ref.object_type) + ", " + to_string(ref.function_type) + ")";
    }
    std::string qualified_symbol_stringifier::operator()(primitive_type_integer_reference const& ref) const
    {
        return (ref.has_sign ? "I" : "U") + std::to_string(ref.bits);
    }
    std::string qualified_symbol_stringifier::operator()(primitive_type_bool_reference const& ref) const
    {
        return "BOOL";
    }
    std::string qualified_symbol_stringifier::operator()(value_expression_reference const& ref) const
    {
        return "VALUE";
    }
    std::string qualified_symbol_stringifier::operator()(void_type const&) const
    {
        return "VOID";
    }
    std::string qualified_symbol_stringifier::operator()(module_reference const& ref) const
    {
        return "[[module: " + ref.module_name + "]]";
    }
    std::string qualified_symbol_stringifier::operator()(subdotentity_reference const& ref) const
    {
        return to_string(ref.parent) + "::." + ref.subdotentity_name;
    }
    std::string qualified_symbol_stringifier::operator()(numeric_literal_reference const&) const
    {
        return "NUMERIC_LITERAL";
    }
} // namespace rylang