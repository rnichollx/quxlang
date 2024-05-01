//
// Created by Ryan Nicholl on 11/14/23.
//

#include "quxlang/manipulators/qmanip.hpp"

namespace quxlang
{

    struct qualified_symbol_stringifier : boost::static_visitor< std::string >
    {
        std::string operator()(context_reference const& ref) const;
        std::string operator()(subentity_reference const& ref) const;
        std::string operator()(instance_pointer_type const& ref) const;
        std::string operator()(instanciation_reference const& ref) const;
        std::string operator()(mvalue_reference const& ref) const;
        std::string operator()(tvalue_reference const& ref) const;
        std::string operator()(cvalue_reference const& ref) const;
        std::string operator()(ovalue_reference const& ref) const;
        std::string operator()(module_reference const& ref) const;
        std::string operator()(bound_function_type_reference const& ref) const;
        std::string operator()(primitive_type_integer_reference const& ref) const;
        std::string operator()(primitive_type_bool_reference const& ref) const;
        std::string operator()(value_expression_reference const& ref) const;
        std::string operator()(subdotentity_reference const& ref) const;
        std::string operator()(void_type const&) const;
        std::string operator()(numeric_literal_reference const&) const;
        std::string operator()(avalue_reference const&) const;
        std::string operator()(template_reference const&) const;
        std::string operator()(selection_reference const&) const;
        std::string operator()(function_arg const&) const;

      public:
        qualified_symbol_stringifier() = default;
    };

    std::string to_string(call_type const& ref)
    {
        std::string result = "call_type(";
        bool first = true;
        for (auto const& [name, type] : ref.named_parameters)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += ", ";
            }
            result += "@" + name + " " + to_string(type);
        }
        for (std::size_t i = 0; i < ref.positional_parameters.size(); i++)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += ", ";
            }
            auto typestr = to_string(ref.positional_parameters.at(i));
            assert(!typestr.empty());
            result += typestr;
        }
        result += ")";

        return result;
    }

    bool is_template(type_symbol const& ref);

    struct is_template_visitor : boost::static_visitor< bool >
    {
      public:
        is_template_visitor()
        {
        }

        bool operator()(subentity_reference const& ref) const
        {
            return is_template(ref.parent);
        }

        bool operator()(instance_pointer_type const& ref) const
        {
            return is_template(ref.target);
        }

        bool operator()(instanciation_reference const& ref) const
        {
            if (is_template(ref.callee))
                return true;
            for (auto& p : ref.parameters.positional_parameters)
            {
                if (is_template(p))
                    return true;
            }
            // TODO: Support named parameters here
            return false;
        }

        bool operator()(selection_reference const& ref) const
        {
            if (is_template(ref.callee))
                return true;
            return false;
        }

        bool operator()(mvalue_reference const& ref) const
        {
            return is_template(ref.target);
        }

        bool operator()(tvalue_reference const& ref) const
        {
            return is_template(ref.target);
        }

        bool operator()(cvalue_reference const& ref) const
        {
            return is_template(ref.target);
        }

        bool operator()(ovalue_reference const& ref) const
        {
            return is_template(ref.target);
        }

        bool operator()(context_reference const& ref) const
        {
            return false;
        }

        bool operator()(bound_function_type_reference const& ref) const
        {
            return is_template(ref.object_type) || is_template(ref.function_type);
        }

        bool operator()(primitive_type_integer_reference const& ref) const
        {
            return false;
        }

        bool operator()(primitive_type_bool_reference const& ref) const
        {
            return false;
        }

        bool operator()(value_expression_reference const& ref) const
        {
            return false;
        }

        bool operator()(void_type const&) const
        {
            return false;
        }

        bool operator()(module_reference const& ref) const
        {
            return false;
        }

        bool operator()(subdotentity_reference const& ref) const
        {
            return is_template(ref.parent);
        }

        bool operator()(numeric_literal_reference const&) const
        {
            return false;
        }

        bool operator()(avalue_reference const& ref) const
        {
            return is_template(ref.target);
        }

        bool operator()(template_reference const&) const
        {
            return true;
        }
    };

    bool is_template(type_symbol const& ref)
    {
        return rpnx::apply_visitor< bool >(is_template_visitor{}, ref);
    }

    std::optional< type_symbol > qualified_parent(type_symbol input)
    {
        if (input.type() == boost::typeindex::type_id< subentity_reference >())
        {
            return as< subentity_reference >(input).parent;
        }
        else if (input.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return as< instance_pointer_type >(input).target;
        }
        else if (input.type() == boost::typeindex::type_id< instanciation_reference >())
        {
            return as< instanciation_reference >(input).callee;
        }
        else if (input.type() == boost::typeindex::type_id< selection_reference >())
        {
            return as< selection_reference >(input).callee;
        }
        else if (input.type() == boost::typeindex::type_id< subdotentity_reference >())
        {
            return as< subdotentity_reference >(input).parent;
        }
        else
        {
            return std::nullopt;
        }
    }

    type_symbol with_context(type_symbol const& ref, type_symbol const& context)
    {
        if (ref.type() == boost::typeindex::type_id< context_reference >())
        {
            return context;
        }
        else if (ref.type() == boost::typeindex::type_id< subentity_reference >())
        {
            return subentity_reference{with_context(as< subentity_reference >(ref).parent, context), as< subentity_reference >(ref).subentity_name};
        }
        else if (ref.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return instance_pointer_type{with_context(as< instance_pointer_type >(ref).target, context)};
        }
        else if (ref.type() == boost::typeindex::type_id< instanciation_reference >())
        {
            instanciation_reference output = as< instanciation_reference >(ref);
            output.callee = with_context(output.callee, context);
            for (auto& p : output.parameters.positional_parameters)
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
        return "->" + rpnx::apply_visitor< std::string >(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(instanciation_reference const& ref) const
    {
        std::string output = rpnx::apply_visitor< std::string >(*this, ref.callee) + "@(";
        bool first = true;
        for (auto const& [name, type] : ref.parameters.named_parameters)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + name + " " + to_string(type);
        }
        for (auto& p : ref.parameters.positional_parameters)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += rpnx::apply_visitor< std::string >(*this, p);
        }
        output += ")";
        return output;
    }
    std::string qualified_symbol_stringifier::operator()(mvalue_reference const& ref) const
    {
        return "MUT& " + rpnx::apply_visitor< std::string >(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(tvalue_reference const& ref) const
    {
        return "TEMP& " + rpnx::apply_visitor< std::string >(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(cvalue_reference const& ref) const
    {
        return "CONST& " + rpnx::apply_visitor< std::string >(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(ovalue_reference const& ref) const
    {
        return "OUT&" + rpnx::apply_visitor< std::string >(*this, ref.target);
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

    std::string qualified_symbol_stringifier::operator()(function_arg const& ref) const
    {
        return "TODO";
        // TODO: implement this
    }

    std::string qualified_symbol_stringifier::operator()(selection_reference const& ref) const
    {
        std::string output = rpnx::apply_visitor< std::string >(*this, ref.callee) + "@[";
        bool first = true;
        if (ref.overload.builtin)
        {
            output += "BUILTIN; ";
        }
        for (auto const& arg : ref.overload.call_parameters.named_parameters)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + arg.first + " " + to_string(arg.second);
        }
        for (auto const& arg : ref.overload.call_parameters.positional_parameters)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += to_string(arg);
        }
        output += "]";
        return output;
    }

    std::optional< template_match_results > match_template(type_symbol const& template_type, type_symbol const& type)
    {
        std::optional< template_match_results > results;
        assert(is_canonical(type));

        // Template portion match.
        // e.g. T(t1) matches I32 and sets t1 to I32
        if (typeis< template_reference >(template_type))
        {
            template_match_results output;
            auto name = as< template_reference >(template_type).name;
            if (!name.empty())
            {
                output.matches[name] = type;
            }
            output.type = type;
            results = std::move(output);
            return results;
        }

        // Exact match, e.g. I32 matches I32
        if (template_type == type)
        {
            template_match_results result{};
            result.type = type;
            return result;
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
            auto const& ptr_template = as< instance_pointer_type >(template_type);
            auto const& ptr_type = as< instance_pointer_type >(type);
            auto match = match_template(ptr_template.target, ptr_type.target);
            if (!match.has_value())
            {
                return std::nullopt;
            }

            match->type = instance_pointer_type{std::move(match->type)};
            return match;
        }
        else if (typeis< subentity_reference >(template_type))
        {
            auto const& sub_template = as< subentity_reference >(template_type);
            auto const& sub_type = as< subentity_reference >(type);
            if (sub_template.subentity_name != sub_type.subentity_name)
            {
                return std::nullopt;
            }
            auto match = match_template(sub_template.parent, sub_type.parent);
            if (!match.has_value())
            {
                return std::nullopt;
            }
            match->type = subentity_reference{std::move(match->type), sub_template.subentity_name};
            return match;
        }
        else if (typeis< subdotentity_reference >(template_type))
        {
            // it is not possible for a type to be a subdotentity_reference
            throw std::logic_error("::. cannot appear in an argument type or argument type template");
        }
        else if (typeis< cvalue_reference >(template_type))
        {
            cvalue_reference const& template_cval = as< cvalue_reference >(template_type);
            cvalue_reference const& type_cval = as< cvalue_reference >(type);
            auto match = match_template(template_cval.target, type_cval.target);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = cvalue_reference{std::move(match->type)};
            return match;
        }
        else if (typeis< mvalue_reference >(template_type))
        {
            mvalue_reference const& template_mval = as< mvalue_reference >(template_type);
            mvalue_reference const& type_mval = as< mvalue_reference >(type);
            auto match = match_template(template_mval.target, type_mval.target);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = mvalue_reference{std::move(match->type)};
            return match;
        }
        else if (typeis< ovalue_reference >(template_type))
        {
            ovalue_reference const& template_oval = as< ovalue_reference >(template_type);
            ovalue_reference const& type_oval = as< ovalue_reference >(type);
            auto match = match_template(template_oval.target, type_oval.target);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = ovalue_reference{std::move(match->type)};
            return match;
        }
        else if (typeis< tvalue_reference >(template_type))
        {
            tvalue_reference const& template_tval = as< tvalue_reference >(template_type);
            tvalue_reference const& type_tval = as< tvalue_reference >(type);
            auto match = match_template(template_tval.target, type_tval.target);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = tvalue_reference{std::move(match->type)};
            return match;
        }
        else if (typeis< instanciation_reference >(template_type))
        {
            instanciation_reference const& template_funct = as< instanciation_reference >(template_type);
            instanciation_reference const& type_funct = as< instanciation_reference >(type);

            if (template_funct.parameters.positional_parameters.size() != type_funct.parameters.positional_parameters.size())
            {
                return std::nullopt;
            }

            // TODO: support named parameters

            auto callee_match = match_template(template_funct.callee, type_funct.callee);
            if (!callee_match)
            {
                return std::nullopt;
            }

            template_match_results all_results;
            all_results.type = std::move(callee_match->type);
            all_results.matches = std::move(callee_match->matches);

            std::vector< type_symbol > instanciated_parameters;
        }

        // In other cases, we are talking about a non-composite reference
        // However, we should make sure we don't miss types

        assert(typeis< primitive_type_integer_reference >(template_type) || typeis< primitive_type_bool_reference >(template_type) || typeis< void_type >(template_type) || typeis< module_reference >(template_type) || typeis< numeric_literal_reference >(template_type));
        return std::nullopt;
    }
    std::string to_string(type_symbol const& ref)
    {
        return rpnx::apply_visitor< std::string >(qualified_symbol_stringifier{}, ref);
    }

} // namespace quxlang