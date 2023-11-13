//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_MANIPULATORS_QUALIFIED_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_MANIPULATORS_QUALIFIED_REFERENCE_HEADER

#include "rylang/data/call_overload_set.hpp"
#include "rylang/data/qualified_reference.hpp"
#include "rylang/variant_utils.hpp"

namespace rylang
{

    std::string to_string(qualified_symbol_reference const& ref);

    inline std::string to_string(call_overload_set const& ref)
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

    struct qualified_symbol_stringifier : boost::static_visitor< std::string >
    {
        std::string operator()(context_reference const& ref) const
        {
            return "context";
        }
        std::string operator()(subentity_reference const& ref) const
        {
            return boost::apply_visitor(*this, ref.parent) + "::" + ref.subentity_name;
        }
        std::string operator()(pointer_to_reference const& ref) const
        {
            return "*" + boost::apply_visitor(*this, ref.target);
        }
        std::string operator()(parameter_set_reference const& ref) const
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
        std::string operator()(mvalue_reference const& ref) const
        {
            return "MUT& " + boost::apply_visitor(*this, ref.target);
        }
        std::string operator()(tvalue_reference const& ref) const
        {
            return "TEMP& " + boost::apply_visitor(*this, ref.target);
        }
        std::string operator()(cvalue_reference const& ref) const
        {
            return "CONST& " + boost::apply_visitor(*this, ref.target);
        }
        std::string operator()(ovalue_reference const& ref) const
        {
            return "OUT&" + boost::apply_visitor(*this, ref.target);
        }

        std::string operator()(module_reference const& ref) const
        {
            return "[[module: " + ref.module_name + "]]";
        }

        std::string operator()(bound_function_type_reference const& ref) const
        {
            return "BINDING(" + to_string(ref.object_type) + ", " + to_string(ref.function_type) + ")";
        }

        std::string operator()(primitive_type_integer_reference const& ref) const
        {
            return (ref.has_sign ? "I" : "U") + std::to_string(ref.bits);
        }

        std::string operator()(primitive_type_bool_reference const& ref) const
        {
            return "BOOL";
        }

        std::string operator()(value_expression_reference const& ref) const
        {
            return "VALUE";
        }

        std::string operator()(void_type const&) const
        {
            return "VOID";
        }

      public:
        qualified_symbol_stringifier() = default;
    };

    inline std::string to_string(qualified_symbol_reference const& ref)
    {
        return boost::apply_visitor(qualified_symbol_stringifier{}, ref);
    }

    inline qualified_symbol_reference make_mref(qualified_symbol_reference ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return std::move(ref);
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return mvalue_reference{boost::get< tvalue_reference >(ref).target};
        }
        else if (typeis< cvalue_reference >(ref))
        {
            throw std::runtime_error("cannot convert cvalue to mvalue");
        }
        else if (typeis< ovalue_reference >(ref))
        {
            return mvalue_reference{boost::get< ovalue_reference >(ref).target};
        }
        else
        {
            return mvalue_reference{std::move(ref)};
        }
    }

    inline bool is_ref(qualified_symbol_reference type)
    {
        if (typeis< mvalue_reference >(type))
        {
            return true;
        }
        else if (typeis< tvalue_reference >(type))
        {
            return true;
        }
        else if (typeis< cvalue_reference >(type))
        {
            return true;
        }
        else if (typeis< ovalue_reference >(type))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    inline bool is_ptr(qualified_symbol_reference type)
    {
        if (typeis< pointer_to_reference >(type))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    inline qualified_symbol_reference remove_ref(qualified_symbol_reference type)
    {
        if (typeis< mvalue_reference >(type))
        {
            return boost::get< mvalue_reference >(type).target;
        }
        else if (typeis< tvalue_reference >(type))
        {
            return boost::get< tvalue_reference >(type).target;
        }
        else if (typeis< cvalue_reference >(type))
        {
            return boost::get< cvalue_reference >(type).target;
        }
        else if (typeis< ovalue_reference >(type))
        {
            return boost::get< ovalue_reference >(type).target;
        }
        else
        {
            return type;
        }
    }

    std::optional< qualified_symbol_reference > qualified_parent(qualified_symbol_reference input);

    inline bool qualified_is_contextual(qualified_symbol_reference const& ref);
    inline bool is_contextual(qualified_symbol_reference const& ref)
    {
        return qualified_is_contextual(ref);
    }

    inline bool is_canonical(qualified_symbol_reference const& ref)
    {
        return !qualified_is_contextual(ref);
    }

    inline bool qualified_is_contextual(qualified_symbol_reference const& ref)
    {
        if (ref.type() == boost::typeindex::type_id< context_reference >())
        {
            return true;
        }
        else if (auto parent = qualified_parent(ref); !parent.has_value())
        {
            return false;
        }
        else
        {
            auto parent2 = qualified_parent(ref);
            assert(parent2.has_value());
            return qualified_is_contextual(parent2.value());
        }
    }

    inline std::optional< qualified_symbol_reference > qualified_parent(qualified_symbol_reference input)
    {
        if (input.type() == boost::typeindex::type_id< subentity_reference >())
        {
            return boost::get< subentity_reference >(input).parent;
        }
        else if (input.type() == boost::typeindex::type_id< pointer_to_reference >())
        {
            return boost::get< pointer_to_reference >(input).target;
        }
        else if (input.type() == boost::typeindex::type_id< parameter_set_reference >())
        {
            return boost::get< parameter_set_reference >(input).callee;
        }
        else
        {
            return std::nullopt;
        }
    }

    inline qualified_symbol_reference with_context(qualified_symbol_reference const& ref, qualified_symbol_reference const& context)
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
        else if (ref.type() == boost::typeindex::type_id< parameter_set_reference >())
        {
            parameter_set_reference output = boost::get< parameter_set_reference >(ref);
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

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUALIFIED_REFERENCE_HEADER
