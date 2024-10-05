//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef QUXLANG_MANIPULATORS_QMANIP_HEADER_GUARD
#define QUXLANG_MANIPULATORS_QMANIP_HEADER_GUARD

#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/variant_utils.hpp"

namespace quxlang
{
    namespace vmir2
    {
        struct invocation_args;
    }
    std::string to_string(vmir2::invocation_args const& ref);
    std::string to_string(type_symbol const& ref);

    std::string to_string(call_type const& ref);
    bool is_ref(type_symbol type);

    bool is_template(type_symbol const& ref);

    struct template_match_results
    {
        std::map< std::string, type_symbol > matches;
        type_symbol type;
    };

    std::optional< template_match_results > match_template(type_symbol const& template_type, type_symbol const& type);

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

    std::string to_string(type_symbol const& ref);
    type_symbol make_mref(type_symbol ref);

     type_symbol remove_ref(type_symbol type);

    // Gets the type of
    inline type_symbol load_type(type_symbol t)
    {
        // When loading a reference like a or b, there are two cases,
        // case 1 is that it is a reference, then we load the value as-is,
        // case 2 is that it is a value, then we load the value as a reference.
        // for case 2, that should be a mutable reference.
        if (!is_ref(t))
        {
            return make_mref(t);
        }
        else
        {
            return t;
        }
    }

    inline type_symbol make_mref(type_symbol ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return std::move(ref);
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return mvalue_reference{ref.template get_as< tvalue_reference >().target};
        }
        else if (typeis< cvalue_reference >(ref))
        {
            throw std::logic_error("cannot convert cvalue to mvalue");
        }
        else if (typeis< wvalue_reference >(ref))
        {
            return mvalue_reference{ref.get_as< wvalue_reference >().target};
        }
        else if (typeis< nvalue_slot >(ref))
        {
            return mvalue_reference{ref.get_as< nvalue_slot >().target};
        }
        else
        {
            return mvalue_reference{std::move(ref)};
        }
    }

    inline type_symbol make_oref(type_symbol ref)
    {
        return wvalue_reference{.target = remove_ref(ref)};
    }

    inline type_symbol make_tref(type_symbol ref)
    {
       return tvalue_reference{.target=remove_ref(ref)};
    }

    inline type_symbol make_cref(type_symbol ref)
    {
        return cvalue_reference{.target = remove_ref(ref)};
    }

    inline type_symbol create_nslot(type_symbol ref)
    {
        assert(!typeis<nvalue_slot>(ref));
        return nvalue_slot{ref};
    }

    bool is_ref(type_symbol type);



    inline bool is_ref_implicitly_convertible_by_syntax(type_symbol from, type_symbol to)
    {
        assert(is_ref(from));
        assert(is_ref(to));

        if (remove_ref(from) != remove_ref(to))
        {
            return false;
        }

        // Mutable references can be converted to all other references
        if (typeis< mvalue_reference >(from))
        {
            return true;
        }

        // Const references are read only, and can be derived from everything except
        // output-only references (wvalue_reference)
        if (typeis< cvalue_reference >(to) && !typeis< wvalue_reference >(from))
        {
            return true;
        }

        // All references are writable except for const references,
        // although writable, we disallow implied conversion from TEMP& to WRITE&
        // for practical reasons.
        if (typeis< wvalue_reference >(to) && !typeis< cvalue_reference >(from))
        {
            return true;
        }

        // Other conversions that are syntax allowed (like TEMP& to MUT&),
        // are only allowed explicitly, and not implicitly.

        // Other implicit conversions, such as derived class to base class, are semantic
        // conversions covered by semantic rules rather than syntax rules.
        return false;
    }

    inline bool is_ref(type_symbol type)
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
        else if (typeis< wvalue_reference >(type))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    inline bool is_ptr(type_symbol type)
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

    inline type_symbol recast_reference(type_symbol obj, type_symbol field_type)
    {
        if (is_ref(field_type))
        {
            return field_type;
        }
        else if (typeis< mvalue_reference >(obj))
        {
            return make_mref(field_type);
        }
        else if (typeis< tvalue_reference >(obj))
        {
            return make_tref(field_type);
        }
        else if (typeis< cvalue_reference >(obj))
        {
            return make_cref(field_type);
        }
        else if (typeis< wvalue_reference >(obj))
        {
            return make_oref(field_type);
        }
        else
        {
            // throw rpnx::unimplemented();
            throw "unimplemented";
            // shouldn't get here?
            return field_type;
        }
    }

    inline type_symbol remove_ref(type_symbol type)
    {
        if (typeis< mvalue_reference >(type))
        {
            return as< mvalue_reference >(type).target;
        }
        else if (typeis< tvalue_reference >(type))
        {
            return as< tvalue_reference >(type).target;
        }
        else if (typeis< cvalue_reference >(type))
        {
            return as< cvalue_reference >(type).target;
        }
        else if (typeis< wvalue_reference >(type))
        {
            return as< wvalue_reference >(type).target;
        }
        else if (typeis< nvalue_slot >(type))
        {
            return as< nvalue_slot >(type).target;
        }
        else if (typeis< dvalue_slot >(type))
        {
            return as< dvalue_slot >(type).target;
        }
        else
        {
            return type;
        }
    }

    std::optional< type_symbol > qualified_parent(type_symbol input);

    inline bool qualified_is_contextual(type_symbol const& ref);
    inline bool is_contextual(type_symbol const& ref)
    {
        return qualified_is_contextual(ref);
    }

    inline bool is_canonical(type_symbol const& ref)
    {
        return !qualified_is_contextual(ref);
    }

    inline bool is_integral(type_symbol const& ref)
    {
        return typeis< primitive_type_integer_reference >(ref);
    }

    inline bool is_numeric_literal(type_symbol const& ref)
    {
        return typeis< numeric_literal_reference >(ref);
    }

    inline bool qualified_is_contextual(type_symbol const& ref)
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

    std::optional< type_symbol > qualified_parent(type_symbol input);

    type_symbol with_context(type_symbol const& ref, type_symbol const& context);

    inline bool is_primitive(type_symbol sym)
    {
        return typeis< primitive_type_integer_reference >(sym) || typeis< primitive_type_bool_reference >(sym) || typeis< instance_pointer_type >(sym);
    }

} // namespace quxlang

#endif // QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD
