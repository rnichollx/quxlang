// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/vmir2/vmir2.hpp"

namespace quxlang
{

    struct qualified_symbol_stringifier : boost::static_visitor< std::string >
    {
        std::string operator()(context_reference const& ref) const;
        std::string operator()(subsymbol const& ref) const;
        std::string operator()(instance_pointer_type const& ref) const;
        std::string operator()(instantiation_type const& ref) const;
        std::string operator()(mvalue_reference const& ref) const;
        std::string operator()(tvalue_reference const& ref) const;
        std::string operator()(cvalue_reference const& ref) const;
        std::string operator()(wvalue_reference const& ref) const;
        std::string operator()(module_reference const& ref) const;
        std::string operator()(bound_type_reference const& ref) const;
        std::string operator()(int_type const& ref) const;
        std::string operator()(bool_type const& ref) const;
        std::string operator()(value_expression_reference const& ref) const;
        std::string operator()(submember const& ref) const;
        std::string operator()(void_type const&) const;

        std::string operator()(thistype const&) const;
        std::string operator()(auto_reference const&) const;

        std::string operator()(numeric_literal_reference const&) const;
        std::string operator()(avalue_reference const&) const;
        std::string operator()(template_reference const&) const;
        std::string operator()(selection_reference const&) const;
        std::string operator()(function_arg const&) const;
        std::string operator()(nvalue_slot const&) const;
        std::string operator()(dvalue_slot const&) const;

      public:
        qualified_symbol_stringifier() = default;
    };

    std::string to_string(vmir2::invocation_args const& ref)
    {
        std::string result = "[";
        bool first = true;
        for (auto const& [name, arg] : ref.named)
        {
            if (first)
                first = false;
            else
                result += ", ";
            result += "@" + name + " " + std::to_string(arg);
        }
        for (auto const& arg : ref.positional)
        {
            if (first)
                first = false;
            else
                result += ", ";
            result += std::to_string(arg);
        }
        result += "]";
        return result;
    }



    bool is_template(type_symbol const& ref);

    struct is_template_visitor
    {
      public:
        is_template_visitor()
        {
        }

        bool operator()(subsymbol const& ref) const
        {
            return is_template(ref.of);
        }

        bool operator()(instance_pointer_type const& ref) const
        {
            return is_template(ref.target);
        }

        bool operator()(nvalue_slot const&) const
        {
            return false;
        }

        bool operator()(instantiation_type const& ref) const
        {
            if (is_template(ref.callee))
                return true;
            for (auto& p : ref.parameters.positional)
            {
                if (is_template(p))
                    return true;
            }
            // TODO: Support named parameters here
            return false;
        }

        bool operator()(selection_reference const& ref) const
        {
            if (is_template(ref.templexoid))
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

        bool operator()(wvalue_reference const& ref) const
        {
            return is_template(ref.target);
        }


        bool operator()(auto_reference const& ref) const
        {
            return true;
        }


        bool operator()(context_reference const& ref) const
        {
            return false;
        }

        bool operator()(bound_type_reference const& ref) const
        {
            return is_template(ref.carried_type) || is_template(ref.bound_symbol);
        }

        bool operator()(int_type const& ref) const
        {
            return false;
        }

        bool operator()(bool_type const& ref) const
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

        bool operator()(submember const& ref) const
        {
            return is_template(ref.of);
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

        bool operator()(dvalue_slot const& t) const
        {
            return is_template(t.target);
        }

        bool operator()(thistype const & t) const
        {
            return false;
        }
    };

    bool is_template(type_symbol const& ref)
    {
        return rpnx::apply_visitor< bool >(is_template_visitor{}, ref);
    }

    std::optional< type_symbol > qualified_parent(type_symbol input)
    {
        if (input.type() == boost::typeindex::type_id< subsymbol >())
        {
            return as< subsymbol >(input).of;
        }
        else if (input.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return as< instance_pointer_type >(input).target;
        }
        else if (input.type() == boost::typeindex::type_id< instantiation_type >())
        {
            return as< instantiation_type >(input).callee;
        }
        else if (input.type() == boost::typeindex::type_id< selection_reference >())
        {
            return as< selection_reference >(input).templexoid;
        }
        else if (input.type() == boost::typeindex::type_id< submember >())
        {
            return as< submember >(input).of;
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
        else if (ref.type() == boost::typeindex::type_id< subsymbol >())
        {
            return subsymbol{with_context(as< subsymbol >(ref).of, context), as< subsymbol >(ref).name};
        }
        else if (ref.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            return instance_pointer_type{with_context(as< instance_pointer_type >(ref).target, context)};
        }
        else if (ref.type() == boost::typeindex::type_id< instantiation_type >())
        {
            instantiation_type output = as< instantiation_type >(ref);
            output.callee = with_context(output.callee, context);
            for (auto& p : output.parameters.positional)
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

    std::string qualified_symbol_stringifier::operator()(subsymbol const& ref) const
    {
        return to_string(ref.of) + "::" + ref.name;
    }
    std::string qualified_symbol_stringifier::operator()(instance_pointer_type const& ref) const
    {
        return "->" + rpnx::apply_visitor< std::string >(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(instantiation_type const& ref) const
    {
        // There are 3 types of selection/instanciation,
        // only selection #[ ]
        // only instanciation #( )
        // both selection & instantiation #{ }
        if (typeis< selection_reference >(ref.callee))
        {
            selection_reference const& sel = as< selection_reference >(ref.callee);
            std::string output = rpnx::apply_visitor< std::string >(*this, sel.templexoid);

            output += " #{";

            if (sel.overload.builtin)
            {
                output += "BUILTIN; ";
            }
            bool first = true;
            for (auto const& [name, type] : sel.overload.call_parameters.named)
            {
                if (first)
                    first = false;
                else
                    output += ", ";
                output += "@" + name + " " + to_string(type);
                if (ref.parameters.named.at(name) != type)
                {
                    output += ": " + to_string(ref.parameters.named.at(name));
                }
            }
            for (size_t i = 0; i < sel.overload.call_parameters.positional.size(); i++)
            {
                if (first)
                    first = false;
                else
                    output += ", ";
                output += to_string(sel.overload.call_parameters.positional.at(i));
                if (ref.parameters.positional.at(i) != sel.overload.call_parameters.positional.at(i))
                {
                    output += ": " + to_string(ref.parameters.positional.at(i));
                }
            }
            output += "}";
            return output;
        }
        std::string output = rpnx::apply_visitor< std::string >(*this, ref.callee);
        output += " #(";
        bool first = true;
        for (auto const& [name, type] : ref.parameters.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + name + " " + to_string(type);
        }
        for (auto& p : ref.parameters.positional)
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
    std::string qualified_symbol_stringifier::operator()(wvalue_reference const& ref) const
    {
        return "OUT&" + rpnx::apply_visitor< std::string >(*this, ref.target);
    }
    std::string qualified_symbol_stringifier::operator()(context_reference const& ref) const
    {
        return "context";
    }
    std::string qualified_symbol_stringifier::operator()(bound_type_reference const& ref) const
    {
        return "BINDING(" + to_string(ref.carried_type) + ", " + to_string(ref.bound_symbol) + ")";
    }
    std::string qualified_symbol_stringifier::operator()(int_type const& ref) const
    {
        return (ref.has_sign ? "I" : "U") + std::to_string(ref.bits);
    }
    std::string qualified_symbol_stringifier::operator()(bool_type const& ref) const
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

    std::string qualified_symbol_stringifier::operator()(thistype const&) const
    {
        return "THISTYPE";
    }
    std::string qualified_symbol_stringifier::operator()(module_reference const& ref) const
    {
        return "MODULE(" + ref.module_name + ")";
    }
    std::string qualified_symbol_stringifier::operator()(submember const& ref) const
    {
        return to_string(ref.of) + "::." + ref.name;
    }

    std::string qualified_symbol_stringifier::operator()(auto_reference const & ref) const
    {
        return "AUTO& " + to_string(ref.target);
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

    std::string qualified_symbol_stringifier::operator()(nvalue_slot const& ref) const
    {
        return "NEW& " + to_string(ref.target) + "";
    }

    std::string qualified_symbol_stringifier::operator()(dvalue_slot const& ref) const
    {
        return "DESTROY& " + to_string(ref.target) + "";
    }

    std::string qualified_symbol_stringifier::operator()(selection_reference const& ref) const
    {
        std::string output = rpnx::apply_visitor< std::string >(*this, ref.templexoid) + "#[";
        bool first = true;
        if (ref.overload.builtin)
        {
            output += "BUILTIN; ";
        }
        for (auto const& arg : ref.overload.call_parameters.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + arg.first + " " + to_string(arg.second);
        }
        for (auto const& arg : ref.overload.call_parameters.positional)
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

        if (typeis <auto_reference>(template_type))
        {
            // Matches any reference type
            auto_reference const& template_autoref = as< auto_reference >(template_type);


            std::string type_str = to_string(type);

            if (!is_ref(type))
            {
                // AUTO& ... matches any kind of reference
                // However, a PR value should be converted to a TEMP& reference

                template_match_results output;

                type_symbol template_of = make_tref(template_autoref.target);
                return match_template(template_of, make_tref(type));
            }

            auto type_refof = remove_ref(type);

            auto match = match_template(template_autoref.target, type_refof);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = type;
            return match;
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
        // e.g. foo#(I32, MUT& I32) cannot match foo#(I32, CONST& I32)

        // TODO: Add template_arg_matches to the top level to allow builtin implicit conversions


        if (typeis< instance_pointer_type >(template_type))
        {
            auto const& ptr_template = as< instance_pointer_type >(template_type);
            if (!type.type_is<instance_pointer_type>())
            {
                return std::nullopt;
            }
            auto const& ptr_type = as< instance_pointer_type >(type);
            auto match = match_template(ptr_template.target, ptr_type.target);
            if (!match.has_value())
            {
                return std::nullopt;
            }

            match->type = instance_pointer_type{std::move(match->type)};
            return match;
        }
        else if (typeis< subsymbol >(template_type))
        {
            auto const& sub_template = as< subsymbol >(template_type);
            if (!type.type_is<subsymbol>())
            {
                return std::nullopt;
            }
            auto const& sub_type = as< subsymbol >(type);
            if (sub_template.name != sub_type.name)
            {
                return std::nullopt;
            }
            auto match = match_template(sub_template.of, sub_type.of);
            if (!match.has_value())
            {
                return std::nullopt;
            }
            match->type = subsymbol{std::move(match->type), sub_template.name};
            return match;
        }
        else if (typeis< submember >(template_type))
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
        else if (typeis< wvalue_reference >(template_type))
        {
            wvalue_reference const& template_oval = as< wvalue_reference >(template_type);
            wvalue_reference const& type_oval = as< wvalue_reference >(type);
            auto match = match_template(template_oval.target, type_oval.target);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = wvalue_reference{std::move(match->type)};
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
        else if (typeis< instantiation_type >(template_type))
        {
            instantiation_type const& template_funct = as< instantiation_type >(template_type);
            instantiation_type const& type_funct = as< instantiation_type >(type);

            if (template_funct.parameters.positional.size() != type_funct.parameters.positional.size())
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

            // TODO: here
            // rpnx::unimplemented();
        }


        if (template_type.type() != type.type())
        {
            return std::nullopt;
        }

        // In other cases, we are talking about a non-composite reference
        // However, we should make sure we don't miss types

        assert(typeis< int_type >(template_type) || typeis< bool_type >(template_type) || typeis< void_type >(template_type) || typeis< module_reference >(template_type) || typeis< numeric_literal_reference >(template_type));
        return std::nullopt;
    }
    std::string to_string(type_symbol const& ref)
    {
        return rpnx::apply_visitor< std::string >(qualified_symbol_stringifier{}, ref);
    }

    type_symbol get_templexoid(instantiation_type const& ref)
    {
        auto callee = ref.callee;

       assert(callee.type_is< selection_reference >());

        return as< selection_reference >(callee).templexoid;
    }

    std::optional< type_symbol > func_class(type_symbol const& func)
    {
        std::string func_str = to_string(func);
        auto tmplx = get_templexoid(func.get_as< instantiation_type >());
        if (tmplx.type_is< submember >())
        {
            return as< submember >(tmplx).of;
        }
        return std::nullopt;
    }

} // namespace quxlang