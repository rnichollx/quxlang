//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef QUXLANG_MANIPULATORS_QUALIFIED_REFERENCE_HEADER_GUARD
#define QUXLANG_MANIPULATORS_QUALIFIED_REFERENCE_HEADER_GUARD

#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/variant_utils.hpp"

namespace quxlang
{

    std::string to_string(type_symbol const& ref);

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
            throw std::runtime_error("cannot convert cvalue to mvalue");
        }
        else if (typeis< ovalue_reference >(ref))
        {
            return mvalue_reference{ref.get_as< ovalue_reference >().target};
        }
        else
        {
            return mvalue_reference{std::move(ref)};
        }
    }

    inline type_symbol make_oref(type_symbol ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return ovalue_reference{ref.get_as< mvalue_reference >().target};
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return ovalue_reference{as< tvalue_reference >(ref).target};
        }
        else if (typeis< cvalue_reference >(ref))
        {
            return ovalue_reference{as< cvalue_reference >(ref).target};
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

    inline type_symbol make_tref(type_symbol ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return tvalue_reference{as< mvalue_reference >(ref).target};
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return std::move(ref);
        }
        else if (typeis< cvalue_reference >(ref))
        {
            return tvalue_reference{as< cvalue_reference >(ref).target};
        }
        else if (typeis< ovalue_reference >(ref))
        {
            return tvalue_reference{as< ovalue_reference >(ref).target};
        }
        else
        {
            return tvalue_reference{std::move(ref)};
        }
    }

    inline type_symbol make_cref(type_symbol ref)
    {
        if (typeis< mvalue_reference >(ref))
        {
            return cvalue_reference{as< mvalue_reference >(ref).target};
        }
        else if (typeis< tvalue_reference >(ref))
        {
            return cvalue_reference{as< tvalue_reference >(ref).target};
        }
        else if (typeis< cvalue_reference >(ref))
        {
            return std::move(ref);
        }
        else if (typeis< ovalue_reference >(ref))
        {
            return cvalue_reference{as< ovalue_reference >(ref).target};
        }
        else
        {
            return cvalue_reference{std::move(ref)};
        }
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
        else if (typeis< ovalue_reference >(type))
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
        else if (typeis< ovalue_reference >(type))
        {
            return as< ovalue_reference >(type).target;
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
