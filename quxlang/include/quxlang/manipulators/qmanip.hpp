// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_QMANIP_HEADER_GUARD
#define QUXLANG_MANIPULATORS_QMANIP_HEADER_GUARD

#include "quxlang/data/fwd.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/variant_utils.hpp"

namespace quxlang
{
    struct codegen_invocation_args;
    namespace vmir2
    {
        struct invocation_args;
        struct routine_parameters;
    } // namespace vmir2
    std::string to_string(vmir2::invocation_args const& ref);
    std::string to_string(codegen_invocation_args const& ref);
    std::string to_string(type_symbol const& ref);

    std::string to_string(invotype const& ref);
    std::string to_string(vmir2::routine_parameters const& ref);
    std::string to_string(intertype const& ref);
    std::string to_string(argif const& ref);
    std::string to_string(expression const& expr);

    bool is_ref(type_symbol type);

    bool is_template(type_symbol const& ref);

    struct template_match_results
    {
        std::map< std::string, type_symbol > matches;
        type_symbol type;
    };

    std::optional< template_match_results > match_template(type_symbol const& template_type, type_symbol const& type);
    std::optional< template_match_results > match_template2(type_symbol const& template_type, type_symbol const& type);
    std::optional< template_match_results > match_template_noconv2(type_symbol const& template_type, type_symbol const& type);

    inline auto knot(context_reference) -> std::tuple<>
    {
        return {};
    }

    inline auto knot(subsymbol& ref)
    {
        return std::tie(ref.of);
    }

    inline auto knot(subsymbol const& ref)
    {
        return std::tie(ref.of);
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
        if (typeis< ptrref_type >(ref))
        {
            if (ref.get_as< ptrref_type >().ptr_class == pointer_class::ref)
            {
                auto const& pref = ref.get_as< ptrref_type >();
                if (pref.qual == qualifier::constant)
                {
                    throw std::logic_error("can't convert from CONST to MUT");
                }
                return ptrref_type{.target = pref.target, .ptr_class = pointer_class::ref, .qual = qualifier::mut};
            }
            return ptrref_type{.target = ref, .ptr_class = pointer_class::ref, .qual = qualifier::mut};
        }
        else if (typeis< nvalue_slot >(ref))
        {
            return ptrref_type{.target = ref.get_as< nvalue_slot >().target, .ptr_class = pointer_class::ref, .qual = qualifier::mut};
        }
        else
        {
            return ptrref_type{.target = std::move(ref), .ptr_class = pointer_class::ref, .qual = qualifier::mut};
        }
    }

    inline type_symbol make_wref(type_symbol ref)
    {
        return ptrref_type{.target = remove_ref(ref), .ptr_class = pointer_class::ref, .qual = qualifier::write};
    }

    inline type_symbol make_tref(type_symbol ref)
    {
        return ptrref_type{.target = remove_ref(ref), .ptr_class = pointer_class::ref, .qual = qualifier::temp};
    }

    inline type_symbol make_cref(type_symbol ref)
    {
        return ptrref_type{.target = remove_ref(ref), .ptr_class = pointer_class::ref, .qual = qualifier::constant};
    }

    inline type_symbol create_nslot(type_symbol ref)
    {
        assert(!typeis< nvalue_slot >(ref));
        return nvalue_slot{ref};
    }

    bool is_ref(type_symbol type);

    inline bool is_ref_implicitly_convertible_by_syntax(type_symbol from, type_symbol to)
    {
        QUXLANG_ASSERT(is_ref(from));
        QUXLANG_ASSERT(is_ref(to));

        if (remove_ref(from) != remove_ref(to))
        {
            return false;
        }

        // Mutable references can be converted to all other references
        if (typeis< ptrref_type >(from) && from.get_as< ptrref_type >().qual == qualifier::mut)
        {
            return true;
        }

        ptrref_type const& from_ptr = from.get_as< ptrref_type >();
        ptrref_type const& to_ptr = to.get_as< ptrref_type >();

        // Const references are read only, and can be derived from everything except
        // output-only references (wvalue_reference)
        if (to_ptr.qual == qualifier::constant && from_ptr.qual != qualifier::write)
        {
            return true;
        }

        // All references are writable except for const references,
        // although writable, we disallow implied conversion from TEMP& to WRITE&
        // for practical reasons.
        if (to_ptr.qual == qualifier::write && from_ptr.qual != qualifier::constant && from_ptr.qual != qualifier::temp)
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
        if (!typeis< ptrref_type >(type))
        {
            return false;
        }

        auto const& ptrv = type.get_as< ptrref_type >();

        if (ptrv.ptr_class != pointer_class::ref)
        {
            return false;
        }

        return true;
    }

    inline bool is_ptr(type_symbol type)
    {
        if (typeis< ptrref_type >(type))
        {
            return type.get_as< ptrref_type >().ptr_class != pointer_class::ref;
        }
        else
        {
            return false;
        }
    }

    inline type_symbol recast_reference(ptrref_type obj, type_symbol field_type)
    {
        if (typeis< ptrref_type >(field_type) && field_type.get_as< ptrref_type >().ptr_class == pointer_class::ref)
        {
            return field_type;
        }
        else
        {
            return ptrref_type{.target = field_type, .ptr_class = pointer_class::ref, .qual = obj.qual};
        }
    }

    inline type_symbol remove_ptr(type_symbol type)
    {
        if (typeis< ptrref_type >(type))
        {
            return as< ptrref_type >(type).target;
        }
        else
        {
            return type;
        }
    }

    inline bool is_const_ref(type_symbol type)
    {
        if (typeis< ptrref_type >(type))
        {
            return as< ptrref_type >(type).qual == qualifier::constant && as< ptrref_type >(type).ptr_class == pointer_class::ref;
        }
        return false;
    }

    inline bool is_write_ref(type_symbol type)
    {
        if (typeis< ptrref_type >(type))
        {
            return as< ptrref_type >(type).qual == qualifier::write && as< ptrref_type >(type).ptr_class == pointer_class::ref;
        }
        return false;
    }

    inline bool is_temp_ref(type_symbol type)
    {
        if (typeis< ptrref_type >(type))
        {
            return as< ptrref_type >(type).qual == qualifier::temp && as< ptrref_type >(type).ptr_class == pointer_class::ref;
        }
        return false;
    }

    inline bool is_mut_ref(type_symbol type)
    {

        if (typeis< ptrref_type >(type))
        {
            return as< ptrref_type >(type).qual == qualifier::mut && as< ptrref_type >(type).ptr_class == pointer_class::ref;
        }
        return false;
    }

    inline bool is_auto_ref(type_symbol type)
    {
        if (typeis< ptrref_type >(type))
        {
            return as< ptrref_type >(type).qual == qualifier::auto_ && as< ptrref_type >(type).ptr_class == pointer_class::ref;
        }
        return false;
    }

    inline type_symbol remove_ref(type_symbol type)
    {
        if (typeis< ptrref_type >(type))
        {
            if (as< ptrref_type >(type).ptr_class == pointer_class::ref)
            {
                return as< ptrref_type >(type).target;
            }
        }
        else if (typeis< nvalue_slot >(type))
        {
            return as< nvalue_slot >(type).target;
        }
        else if (typeis< dvalue_slot >(type))
        {
            return as< dvalue_slot >(type).target;
        }
        return type;
    }

    std::optional< type_symbol > type_parent(type_symbol input);

    inline bool type_is_contextual(type_symbol const& ref);
    inline bool is_contextual(type_symbol const& ref)
    {
        return type_is_contextual(ref);
    }

    inline bool is_canonical(type_symbol const& ref)
    {
        return !type_is_contextual(ref);
    }

    inline bool is_integral(type_symbol const& ref)
    {
        return typeis< int_type >(ref);
    }

    inline bool is_numeric_literal(type_symbol const& ref)
    {
        return typeis< numeric_literal_reference >(ref);
    }

    inline std::optional<type_symbol> get_root_module(type_symbol const& ref)
    {
        if (ref.type_is< absolute_module_reference >())
        {
            return ref;
        }
        else if (auto parent = type_parent(ref); !parent.has_value())
        {
            return std::nullopt;
        }
        else
        {
            return get_root_module(parent.value());
        }
    }

    inline bool type_is_contextual(type_symbol const& ref)
    {
        if (ref.type_is< context_reference >() || ref.type_is< freebound_identifier >())
        {
            return true;
        }
        else if (auto parent = type_parent(ref); !parent.has_value())
        {
            return false;
        }
        else
        {
            auto parent2 = type_parent(ref);
            assert(parent2.has_value());
            return type_is_contextual(parent2.value());
        }
    }

    std::optional< type_symbol > type_parent(type_symbol input);

    type_symbol with_context(type_symbol const& ref, type_symbol const& context);

    inline bool is_primitive(type_symbol sym)
    {
        return typeis< int_type >(sym) || typeis< bool_type >(sym) || typeis< ptrref_type >(sym) || typeis< readonly_constant >(sym) || typeis< byte_type >(sym);
    }

} // namespace quxlang

#endif // QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD
