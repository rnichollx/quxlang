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
        else if (input.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return boost::get< instance_pointer_type >(input).target;
        }
        else if (input.type() == boost::typeindex::type_id< functanoid_reference >())
        {
            return boost::get< functanoid_reference >(input).callee;
        }
        else if (input.type() == boost::typeindex::type_id< subdotentity_reference >())
        {
            return boost::get< subdotentity_reference >(input).parent;
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
        else if (ref.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return instance_pointer_type{with_context(boost::get< instance_pointer_type >(ref).target, context)};
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
    std::string qualified_symbol_stringifier::operator()(instance_pointer_type const& ref) const
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
    std::string qualified_symbol_stringifier::operator()(avalue_reference const& val) const
    {
        return "ARGUMENT& " + to_string(val.target);
    }
    std::string qualified_symbol_stringifier::operator()(template_reference const& val) const
    {
        return "T(" + val.name + ")";
    }
    std::optional< template_match_results > match_template(qualified_symbol_reference const& template_type, qualified_symbol_reference const& type)
    {
        std::optional< template_match_results > results;
        assert(is_canonical(type));

        // Template portion match.
        // e.g. T(t1) matches I32 and sets t1 to I32
        if (typeis< template_reference >(template_type))
        {
            template_match_results output;
            auto name = boost::get< template_reference >(template_type).name;
            if (!name.empty())
            {
                output.matches[name] = type;
            }
            results = std::move(output);
            return results;
        }

        // Exact match, e.g. I32 matches I32
        if (template_type == type)
        {
            return template_match_results{};
        }

        // Impossible to match in this case, e.g. a pointer reference cannot match an integer
        // Note that template matching doesn't do any implicit conversions.
        // So for example, MUT& I32 cannot match a template CONST& I32,
        // even though a MUT& I32 can be implicitly converted to a CONST& I32.
        // This is because might be matching a template parameter, and implementations might differ,
        // so we can't assume that the conversion is valid.
        // For example, a structure might implement differently depending on the mutability of the reference.
        // Such as including a mutex in the MUT& version, but not in the CONST& version.
        // e.g. foo@(I32, MUT& I32) cannot match foo@(I32, CONST& I32)

        // TODO: Add template_arg_matches to the top level to allow builtin implicit conversions

        if (template_type.type() != type.type())
        {
            return std::nullopt;
        }

        if (typeis< instance_pointer_type >(template_type))
        {
            auto const& ptr_template = boost::get< instance_pointer_type >(template_type);
            auto const& ptr_type = boost::get< instance_pointer_type >(type);
            return match_template(ptr_template.target, ptr_type.target);
        }
        else if (typeis< subentity_reference >(template_type))
        {
            auto const& sub_template = boost::get< subentity_reference >(template_type);
            auto const& sub_type = boost::get< subentity_reference >(type);
            if (sub_template.subentity_name != sub_type.subentity_name)
            {
                return std::nullopt;
            }
            return match_template(sub_template.parent, sub_type.parent);
        }
        else if (typeis< subdotentity_reference >(template_type))
        {
            // it is not possible for a type to be a subdotentity_reference
            throw std::logic_error("::. cannot appear in an argument type or argument type template");
        }
        else if (typeis< cvalue_reference >(template_type))
        {
            cvalue_reference const& template_cval = boost::get< cvalue_reference >(template_type);
            cvalue_reference const& type_cval = boost::get< cvalue_reference >(type);
            return match_template(template_cval.target, type_cval.target);
        }
        else if (typeis< mvalue_reference >(template_type))
        {
            mvalue_reference const& template_mval = boost::get< mvalue_reference >(template_type);
            mvalue_reference const& type_mval = boost::get< mvalue_reference >(type);
            return match_template(template_mval.target, type_mval.target);
        }
        else if (typeis< ovalue_reference >(template_type))
        {
            ovalue_reference const& template_oval = boost::get< ovalue_reference >(template_type);
            ovalue_reference const& type_oval = boost::get< ovalue_reference >(type);
            return match_template(template_oval.target, type_oval.target);
        }
        else if (typeis< tvalue_reference >(template_type))
        {
            tvalue_reference const& template_tval = boost::get< tvalue_reference >(template_type);
            tvalue_reference const& type_tval = boost::get< tvalue_reference >(type);
            return match_template(template_tval.target, type_tval.target);
        }


        // In other cases, we are talking about a non-composite reference
        // However, we should make sure we don't miss types

        assert(typeis< primitive_type_integer_reference >(template_type) || typeis< primitive_type_bool_reference >(template_type)  || typeis< void_type >(template_type) || typeis< module_reference >(template_type) || typeis< numeric_literal_reference >(template_type));
        return std::nullopt;
    }

} // namespace rylang