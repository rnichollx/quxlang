//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_MANIPULATORS_QUALIFIED_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_MANIPULATORS_QUALIFIED_REFERENCE_HEADER

#include "rylang/data/call_parameter_information.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/variant_utils.hpp"

namespace rylang
{

    std::string to_string(qualified_symbol_reference const& ref);

    struct call_parameter_information;

    std::string to_string(call_parameter_information const& ref);

    struct qualified_symbol_stringifier : boost::static_visitor< std::string >
    {
        std::string operator()(context_reference const& ref) const;
        std::string operator()(subentity_reference const& ref) const;
        std::string operator()(instance_pointer_type const& ref) const;
        std::string operator()(functanoid_reference const& ref) const;
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
        std::string operator()(template_reference const &) const;

      public:
        qualified_symbol_stringifier() = default;
    };


    struct template_match_results
    {
        std::map<std::string, qualified_symbol_reference > matches;
    };

    std::optional< template_match_results > match_template(qualified_symbol_reference const& template_type, qualified_symbol_reference const& type);

    inline auto knot(context_reference) -> std::tuple<>
    {
        return {};
    }

    inline auto knot(subentity_reference& ref)
    {
        return std::tie(ref.parent);
    }

    inline auto knot(subentity_reference const& ref)
    {
        return std::tie(ref.parent);
    }

    inline auto knot(cvalue_reference const& ref)
    {
        return std::tie(ref.target);
    }



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

    inline qualified_symbol_reference make_oref(qualified_symbol_reference ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return ovalue_reference{boost::get< mvalue_reference >(ref).target};
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return ovalue_reference{boost::get< tvalue_reference >(ref).target};
        }
        else if (typeis< cvalue_reference >(ref))
        {
            return ovalue_reference{boost::get< cvalue_reference >(ref).target};
        }
        else if (typeis< ovalue_reference >(ref))
        {
            return std::move(ref);
        }
        else
        {
            return ovalue_reference{std::move(ref)};
        }
    }

    inline qualified_symbol_reference make_tref(qualified_symbol_reference ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return tvalue_reference{boost::get< mvalue_reference >(ref).target};
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return std::move(ref);
        }
        else if (typeis< cvalue_reference >(ref))
        {
            return tvalue_reference{boost::get< cvalue_reference >(ref).target};
        }
        else if (typeis< ovalue_reference >(ref))
        {
            return tvalue_reference{boost::get< ovalue_reference >(ref).target};
        }
        else
        {
            return tvalue_reference{std::move(ref)};
        }
    }

    inline qualified_symbol_reference make_cref(qualified_symbol_reference ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return cvalue_reference{boost::get< mvalue_reference >(ref).target};
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return cvalue_reference{boost::get< tvalue_reference >(ref).target};
        }
        else if (typeis< cvalue_reference >(ref))
        {
            return std::move(ref);
        }
        else if (typeis< ovalue_reference >(ref))
        {
            return cvalue_reference{boost::get< ovalue_reference >(ref).target};
        }
        else
        {
            return cvalue_reference{std::move(ref)};
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
        if (typeis< instance_pointer_type >(type))
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



    inline bool is_integral(qualified_symbol_reference const& ref)
    {
        return typeis< primitive_type_integer_reference >(ref);
    }

    inline bool is_numeric_literal(qualified_symbol_reference const& ref)
    {
        return typeis< numeric_literal_reference >(ref);
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

    std::optional< qualified_symbol_reference > qualified_parent(qualified_symbol_reference input);

    qualified_symbol_reference with_context(qualified_symbol_reference const& ref, qualified_symbol_reference const& context);

    inline bool is_primitive(qualified_symbol_reference sym)
    {
        return typeis< primitive_type_integer_reference >(sym) || typeis< primitive_type_bool_reference >(sym) || typeis< instance_pointer_type >(sym);
    }

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUALIFIED_SYMBOL_REFERENCE_HEADER
