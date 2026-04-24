// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_BASIC_TYPES_HEADER_GUARD
#define QUXLANG_DATA_BASIC_TYPES_HEADER_GUARD

#include <quxlang/data/lookup_chain.hpp>
#include <quxlang/data/numeric_literal.hpp>
#include <quxlang/data/symbol_type.hpp>
#include <quxlang/cow.hpp>
#include <quxlang/macros.hpp>
#include <rpnx/variant.hpp>
#include <quxlang/variant_utils.hpp>

#include <quxlang/data/fwd.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <rpnx/compare.hpp>
#include <rpnx/macros.hpp>

RPNX_ENUM(quxlang, overload_class, std::uint16_t, user_defined, builtin, intrinsic);

RPNX_ENUM(quxlang, qualifier, std::uint16_t, mut, constant, temp, write, auto_, input, output);
RPNX_ENUM(quxlang, pointer_class, std::uint16_t, instance, array, machine, ref);

RPNX_ENUM(quxlang, constant_kind, std::uint16_t, data, numeric, string, cstring);
RPNX_ENUM(quxlang, allowed_adaptations, std::uint8_t, source_rebinding, class_conversions, destination_rebinding, none);
RPNX_ENUM(quxlang, conversion_type, std::uint8_t, implicit, explicit_, partial, assume, checked);
RPNX_ENUM(quxlang, builtin_allocator_kind, std::uint8_t, constexpr_alloc, constexpr_alloc_multiple, constexpr_dealloc, constexpr_dealloc_multiple);

RPNX_ENUM(quxlang, integral_qualifier, std::uint8_t, none, signed_, unsigned_);
RPNX_ENUM(quxlang, template_parameter_kind, std::uint8_t, type, value);

namespace quxlang
{
    std::optional< pointer_class > pointer_class_template_match(pointer_class template_class, pointer_class match_class);
    std::optional< qualifier > qualifier_template_match(qualifier to_qual, qualifier from_qual);

    std::optional< qualifier > qualifier_template_match_noconv(qualifier template_qual, qualifier match_qual);

    struct antestatal_struct;
    struct antestatal_array;
    struct antestatal_ptrref;
    struct antestatal_primitive;

    using antestatal_value = rpnx::variant< antestatal_primitive, antestatal_array, antestatal_ptrref, antestatal_struct >;

    struct antestatal_access_global;
    struct antestatal_access_field;
    struct antestatal_access_array_element;
    struct antestatal_nullptr;

    using antestatal_access = rpnx::variant< antestatal_nullptr, antestatal_access_global, antestatal_access_array_element, antestatal_access_field >;

    struct antestatal_nullptr
    {
        RPNX_EMPTY_METADATA(antestatal_nullptr);
    };

    struct antestatal_access_global
    {
        type_symbol symbol;

        RPNX_MEMBER_METADATA(antestatal_access_global, symbol);
    };

    struct antestatal_access_field
    {
        antestatal_access object;
        std::string field_name;

        RPNX_MEMBER_METADATA(antestatal_access_field, object, field_name);
    };

    struct antestatal_access_array_element
    {
        antestatal_access array;
        std::uint64_t index = 0;

        RPNX_MEMBER_METADATA(antestatal_access_array_element, array, index);
    };

    struct antestatal_ptrref
    {
        antestatal_access target;

        RPNX_MEMBER_METADATA(antestatal_ptrref, target);
    };

    struct antestatal_primitive
    {
        std::vector< std::byte > value;

        RPNX_MEMBER_METADATA(antestatal_primitive, value);
    };

    struct antestatal_struct
    {
        std::map< std::string, antestatal_value > fields;

        RPNX_MEMBER_METADATA(antestatal_struct, fields);
    };

    struct antestatal_array
    {
        std::vector< antestatal_value > elements;

        RPNX_MEMBER_METADATA(antestatal_array, elements);
    };

    struct constexpr_result
    {
        cow< type_symbol > type;
        cow< std::vector< std::byte > > value;

        RPNX_MEMBER_METADATA(constexpr_result, type, value);
    };

    struct constexpr_serialoid
    {
        std::vector< std::byte > bytes;

        RPNX_MEMBER_METADATA(constexpr_serialoid, bytes);
    };

    using constexpr_value = rpnx::variant< antestatal_value, constexpr_serialoid >;

    struct void_type
    {
        RPNX_EMPTY_METADATA(void_type);
    };

    struct initguard_type
    {
        RPNX_EMPTY_METADATA(initguard_type);
    };

    struct initguard_lock_type
    {
        RPNX_EMPTY_METADATA(initguard_lock_type);
    };

    struct constexpr_proxy
    {
        RPNX_EMPTY_METADATA(constexpr_proxy);
    };

    struct intptr
    {
        bool has_sign = false;

        RPNX_MEMBER_METADATA(intptr, has_sign);
    };

    struct thistype
    {
        RPNX_EMPTY_METADATA(thistype);
    };

    struct invotype
    {
        std::map< std::string, type_symbol > named;
        std::vector< type_symbol > positional;

        inline auto size() const
        {
            return positional.size() + named.size();
        }

        RPNX_MEMBER_METADATA(invotype, named, positional);
    };

    struct declared_parameter
    {
        std::optional< std::string > api_name;
        std::optional< std::string > name;
        template_parameter_kind kind = template_parameter_kind::type;
        type_symbol type;
        std::optional< expression > default_value;

        RPNX_MEMBER_METADATA(declared_parameter, api_name, name, kind, type, default_value);
    };

    struct declared_parameters
    {
        std::vector< declared_parameter > positional;
        std::map< std::string, declared_parameter > named;

        RPNX_MEMBER_METADATA(declared_parameters, positional, named);
    };

    struct temploid_header
    {
        declared_parameters params;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(temploid_header, params, priority);
    };

    struct parameter_type
    {
        /// Declared type for this formal parameter or positional pack element.
        type_symbol type;
        /// Optional expression used when the argument is defaulted.
        std::optional< expression > default_value;
        /// True when this formal positional parameter is a variadic pack.
        bool is_pack = false;

        RPNX_MEMBER_METADATA(parameter_type, type, default_value, is_pack);
    };

    struct paratype
    {
        // The "this" special parameter is replaced with a
        // named parameter with the name "THIS"
        std::vector< parameter_type > positional;
        std::map< std::string, parameter_type > named;

        RPNX_MEMBER_METADATA(paratype, positional, named)
    };

    struct param_names
    {
        std::vector< std::optional< std::string > > positional;
        std::map< std::string, std::string > named;

        RPNX_MEMBER_METADATA(param_names, positional, named);
    };

    struct function_arg
    {
        std::string name;
        std::optional< std::string > api_name;
        type_symbol type;

        RPNX_MEMBER_METADATA(function_arg, name, api_name, type)
    };

    struct argif
    {
        /// Accepted type for this parameter or positional pack element.
        type_symbol type;
        /// True when this parameter can be omitted and initialized from a default expression.
        bool is_defaulted = false;
        /// True when this parameter is a variadic pack.
        bool is_pack = false;
        /// True when this argument interface accepts a static value
        bool requires_static_value = false;

        RPNX_MEMBER_METADATA(argif, type, is_defaulted, is_pack, requires_static_value);
    };

    // Interface type - used in things like overload resolution
    struct intertype
    {
        // Positional arguments of the intertype (nameless)
        std::vector< argif > positional;

        // The named arguments of the intertype (e.g. @foo, @bar, @THIS, etc.)
        std::map< std::string, argif > named;

        RPNX_MEMBER_METADATA(intertype, positional, named);
    };





    // Ensig is the portion #[...]
    struct temploid_ensig
    {
        intertype interface;
        std::optional< std::int32_t > priority;
        std::optional< expression > enable_if;

        RPNX_MEMBER_METADATA(temploid_ensig, interface, priority, enable_if);
    };

    struct overload
    {
        bool builtin = false;
        invotype params;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(overload, builtin, params, priority);
    };

    struct signature
    {
        temploid_ensig ensig;
        std::optional< type_symbol > return_type;

        RPNX_MEMBER_METADATA(signature, ensig, return_type);
    };

    struct sigtype
    {
        invotype params;
        std::optional< type_symbol > return_type;

        RPNX_MEMBER_METADATA(sigtype, params, return_type);
    };

    struct numeric_literal_reference
    {
        RPNX_EMPTY_METADATA(numeric_literal_reference);
    };

    struct string_literal_reference
    {
        RPNX_EMPTY_METADATA(string_literal_reference);
    };

    struct readonly_constant
    {
        constant_kind kind;

        RPNX_MEMBER_METADATA(readonly_constant, kind);
    };

    struct context_reference
    {
        RPNX_EMPTY_METADATA(context_reference);
    };

    struct freebound_identifier
    {
        std::string name;

        RPNX_MEMBER_METADATA(freebound_identifier, name);
    };

    struct builtin_symbol
    {
        std::string name;

        RPNX_MEMBER_METADATA(builtin_symbol, name);
    };

    struct array_type
    {
        type_symbol element_type;
        expression element_count;

        RPNX_MEMBER_METADATA(array_type, element_type, element_count);
    };
    struct expression_arg
    {
        std::optional< std::string > name;
        expression value;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_arg, name, value);
    };

    struct auto_temploidic
    {
        std::string name;
        RPNX_MEMBER_METADATA(auto_temploidic, name);
    };

    struct type_temploidic
    {
        std::string name;
        RPNX_MEMBER_METADATA(type_temploidic, name);
    };

    struct attached_type_reference;

    struct absolute_module_reference
    {
        std::string module_name;

        RPNX_MEMBER_METADATA(absolute_module_reference, module_name);
    };

    struct subsymbol
    {
        type_symbol of;
        std::string name;

        RPNX_MEMBER_METADATA(subsymbol, of, name);
    };

    struct submember
    {
        type_symbol of;
        std::string name;

        RPNX_MEMBER_METADATA(submember, of, name);
    };

    struct int_type
    {
        std::size_t bits = 0;
        bool has_sign = false;

        RPNX_MEMBER_METADATA(int_type, bits, has_sign);
    };

    struct byte_type
    {
        RPNX_EMPTY_METADATA(byte_type);
    };

    struct bool_type
    {
        RPNX_EMPTY_METADATA(bool_type);
    };

    struct procedure_type
    {
        std::string calling_convention = "DEFAULT";
        bool is_noexcept = false;
        sigtype signature;

        RPNX_MEMBER_METADATA(procedure_type, calling_convention, is_noexcept, signature);
    };

    struct ptrref_type
    {
        type_symbol target;
        pointer_class ptr_class;
        qualifier qual;

        RPNX_MEMBER_METADATA(ptrref_type, target, ptr_class, qual);
    };

    struct size_type
    {
        RPNX_EMPTY_METADATA(size_type);
    };

    struct value_expression_reference
    {
        // TODO: Implement
        RPNX_EMPTY_METADATA(value_expression_reference);
    };



    struct temploid_reference
    {
        type_symbol templexoid;
        temploid_ensig which;
        RPNX_MEMBER_METADATA(temploid_reference, templexoid, which);
    };



    struct parameter_type_instantiation
    {
        type_symbol type;

        RPNX_MEMBER_METADATA(parameter_type_instantiation, type);
    };

    struct parameter_value_instantiation
    {
        type_symbol type;
        constexpr_value value;

        RPNX_MEMBER_METADATA(parameter_value_instantiation, type, value);
    };

    using parameter_instantiation = rpnx::variant< parameter_type_instantiation, parameter_value_instantiation >;

    struct instatype
    {
        std::map< std::string, parameter_instantiation > named;
        std::vector< parameter_instantiation > positional;

        inline auto size() const
        {
            return positional.size() + named.size();
        }

        RPNX_MEMBER_METADATA(instatype, named, positional);
    };

    inline auto parameter_instantiation_type(parameter_instantiation const& param) -> type_symbol const&
    {
        if (param.template type_is< parameter_type_instantiation >())
        {
            return param.template get_as< parameter_type_instantiation >().type;
        }
        return param.template get_as< parameter_value_instantiation >().type;
    }

    inline auto make_type_instantiation(type_symbol type) -> parameter_instantiation
    {
        return parameter_type_instantiation{.type = std::move(type)};
    }

    inline auto instatype_from_invotype(invotype input) -> instatype
    {
        instatype output;
        for (auto& [name, type] : input.named)
        {
            output.named[std::move(name)] = make_type_instantiation(std::move(type));
        }
        for (auto& type : input.positional)
        {
            output.positional.push_back(make_type_instantiation(std::move(type)));
        }
        return output;
    }

    inline auto invotype_from_instatype(instatype const& input) -> invotype
    {
        invotype output;
        for (auto const& [name, param] : input.named)
        {
            output.named[name] = parameter_instantiation_type(param);
        }
        for (auto const& param : input.positional)
        {
            output.positional.push_back(parameter_instantiation_type(param));
        }
        return output;
    }

    struct ensig_initialization
    {
        temploid_ensig ensig;
        instatype params;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(ensig_initialization, ensig, params, adaptations);
    };

    struct initialization_reference
    {
        type_symbol initializee;
        std::optional< type_symbol > context;
        std::vector< expression_arg > arguments;
        instatype parameters;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(initialization_reference, initializee, context, arguments, parameters, adaptations);
    };

    struct instanciation_reference
    {
        temploid_reference temploid;
        instatype params;
        RPNX_MEMBER_METADATA(instanciation_reference, temploid, params);
    };

    struct nvalue_slot
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(nvalue_slot, target);
    };

    struct dvalue_slot
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(dvalue_slot, target);
    };

    struct attached_type_reference
    {
        type_symbol carrying_type;
        type_symbol attached_symbol;
        RPNX_MEMBER_METADATA(attached_type_reference, carrying_type, attached_symbol);
    };

    struct array_initializer_type
    {
        type_symbol element_type;
        std::uint64_t count;
        RPNX_MEMBER_METADATA(array_initializer_type, element_type, count);
    };

    /// Represents a __STATIC_LOCAL symbol for stable function-local static storage.
    struct static_local_ref
    {
        /// Function or generated routine that owns this function-local static.
        type_symbol functanoid;
        /// Source-level static name captured by this local symbol.
        std::string name;
        /// Declaration occurrence for this name, used to distinguish shadowed/generated locals.
        std::uint64_t generation = 0;

        RPNX_MEMBER_METADATA(static_local_ref, functanoid, name, generation);
    };

    /// Represents a __STATIC_SNAPSHOT symbol for immutable runtime reads of static locals.
    struct static_snapshot_ref
    {
        /// Function or generated routine that owns the source static.
        type_symbol functanoid;
        /// Source-level static name captured by this snapshot symbol.
        std::string name;
        /// Declaration occurrence for this name.
        std::uint64_t generation = 0;
        /// Snapshot occurrence for a particular immutable read/generation point.
        std::uint64_t snapshot_id = 0;

        RPNX_MEMBER_METADATA(static_snapshot_ref, functanoid, name, generation, snapshot_id);
    };

    /// Type expression that resolves to the concrete type of one argument in a positional pack.
    struct pack_arg_type_ref
    {
        /// Source-level positional pack name to inspect.
        std::string pack_name;
        /// Constexpr expression that selects the zero-based pack element.
        expression index;

        RPNX_MEMBER_METADATA(pack_arg_type_ref, pack_name, index);
    };

    struct keyword_symbol
    {
        std::string name;

        RPNX_MEMBER_METADATA(keyword_symbol, name);
    };

    inline auto builtin_allocator_kind_from_name(std::string_view name) -> std::optional< builtin_allocator_kind >
    {
        if (name == "CONSTEXPR_ALLOC")
        {
            return builtin_allocator_kind::constexpr_alloc;
        }
        if (name == "CONSTEXPR_ALLOC_MULTIPLE")
        {
            return builtin_allocator_kind::constexpr_alloc_multiple;
        }
        if (name == "CONSTEXPR_DEALLOC")
        {
            return builtin_allocator_kind::constexpr_dealloc;
        }
        if (name == "CONSTEXPR_DEALLOC_MULTIPLE")
        {
            return builtin_allocator_kind::constexpr_dealloc_multiple;
        }
        return std::nullopt;
    }

    inline auto is_builtin_allocator_name(std::string_view name) -> bool
    {
        return builtin_allocator_kind_from_name(name).has_value();
    }

    inline auto is_builtin_global_functum_name(std::string_view name) -> bool
    {
        return name == "SERIALIZE_UINTANY" || name == "DESERIALIZE_UINTANY" || name == "SERIALIZE_LEB128" || name == "DESERIALIZE_LEB128";
    }

    struct storage
    {
        std::set< type_symbol > storable_types;

        RPNX_MEMBER_METADATA(storage, storable_types);
    };

    struct aligned_storage
    {
        expression size;
        expression align;

        RPNX_MEMBER_METADATA(aligned_storage, size, align);
    };

    struct expression_add;
    struct expression_subtract;
    struct expression_addp;
    struct expression_addw;
    struct expression_call;
    struct expression_lvalue_reference;
    struct expression_copy_assign;
    struct expression_move_assign;
    struct expression_parenthesis;
    struct expression_multibind;

    struct expression_static_choose;
    struct expression_snapshot;

    struct expression_this_reference
    {
        QUXLANG_WITH_SOURCE_LOCATION_EMPTY_METADATA(expression_this_reference);
    };

    struct expression_dotreference;

    struct expression_thisdot_reference
    {
        std::string field_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_thisdot_reference, field_name);
    };

    struct expression_quarrow
    {
        std::string field_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_quarrow, field_name);
    };

    struct expression_symbol_reference
    {
        type_symbol symbol;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_symbol_reference, symbol);
    };

    struct expression_target
    {
        std::string target;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_target, target);
    };

    struct expression_binary
    {
        std::string operator_str;

        expression lhs;
        expression rhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_binary, operator_str, lhs, rhs);
    };

    struct expression_unary_prefix
    {
        std::string operator_str;
        expression rhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_unary_prefix, operator_str, rhs);
    };

    struct expression_unary_postfix
    {
        std::string operator_str;
        expression lhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_unary_postfix, operator_str, lhs);
    };

    struct expression_dotreference
    {
        expression lhs;
        std::string field_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_dotreference, lhs, field_name);
    };

    // Right arrow -> is used to get a reference from a pointer.
    struct expression_rightarrow
    {
        expression lhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_rightarrow, lhs);
    };

    // Left arrow <- is used to get a pointer from a reference.
    struct expression_leftarrow
    {
        expression lhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_leftarrow, lhs);
    };

    struct expression_multibind
    {
        std::string operator_str;
        expression lhs;
        std::vector< expression > bracketed;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_multibind, operator_str, lhs, bracketed);
    };

    struct expression_lvalue_reference
    {
        lookup_chain chain;
    };

    struct expression_equals
    {
        static constexpr const char* name = "equals";
        static constexpr const char* symbol = "==";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_equals const& other) const = default;
    };

    struct expression_not_equals
    {
        static constexpr const char* name = "not_equals";
        static constexpr const char* symbol = "!=";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_not_equals const& other) const = default;
    };

    struct expression_multiply
    {
        static constexpr const char* const symbol = "*";
        static constexpr const char* const name = "multiply";
        static constexpr const int priority = 5;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_multiply const& other) const = default;
    };

    struct expression_divide
    {
        static constexpr const char* const symbol = "/";
        static constexpr const char* const name = "divide";
        static constexpr const int priority = 3;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_divide const& other) const = default;
    };

    struct expression_modulus
    {
        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_modulus const& other) const = default;
    };

    struct expression_subtract
    {
        static constexpr const char* name = "subtract";
        static constexpr const char* symbol = "-";
        static constexpr const int priority = 4;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_subtract const& other) const = default;
    };

    struct expression_move_assign
    {
        static constexpr const char* name = "move_assign";
        static constexpr const char* symbol = ":<";
        static constexpr const int priority = 0;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_move_assign const& other) const = default;
    };

    struct expression_value_keyword
    {
        std::string keyword;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_value_keyword, keyword);
    };



    struct call_initializer
    {
        std::vector< expression_arg > args;
        RPNX_MEMBER_METADATA(call_initializer, args);
    };

    // An expression like ` :[ a, b, c ] ` for array initialization
    // or ` :[ a, b, c, ... ]` for array initialization with default value
    struct array_initializer
    {
        std::vector< expression > args;
        std::optional< expression > default_initalization;
        RPNX_MEMBER_METADATA(array_initializer, args, default_initalization);
    };

    struct assignment_initializer
    {
        expression expr;
        RPNX_MEMBER_METADATA(assignment_initializer, expr);
    };

    struct expression_call
    {
        expression callee;
        std::vector< expression_arg > args;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_call, callee, args);
    };

    struct expression_bits
    {
        type_symbol of_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_bits, of_type);
    };

    struct expression_sizeof
    {
        type_symbol of_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_sizeof, of_type);
    };

    struct expression_is_integral
    {
        type_symbol of_type;
        integral_qualifier qualifier = integral_qualifier::none;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_is_integral, of_type, qualifier);
    };

    struct expression_same_types
    {
        type_symbol lhs_type;
        type_symbol rhs_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_same_types, lhs_type, rhs_type);
    };

    struct expression_is_signed
    {
        type_symbol of_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_is_signed, of_type);
    };

    struct expression_typecast
    {
        expression expr;
        type_symbol to_type;
        std::optional< std::string > keyword;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_typecast, expr, to_type, keyword);
    };

    struct expression_pun
    {
        expression value;
        type_symbol as_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_pun, value, as_type);
    };

    struct expression_place
    {
        expression at;
        type_symbol type;
        std::optional< expression > assign_init;
        std::vector< expression_arg > args;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_place, at, type, assign_init, args);
    };

    struct expression_static_choose
    {
        expression condition;
        expression true_expr;
        expression false_expr;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_static_choose, condition, true_expr, false_expr);
    };

    struct expression_snapshot
    {
        /// Visible function-local static name to freeze for this runtime expression.
        std::string name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_snapshot, name);
    };

    /// Compile-time expression that evaluates to the number of arguments captured by a positional pack.
    struct expression_pack_size
    {
        /// Source-level positional pack name to inspect.
        std::string pack_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_pack_size, pack_name);
    };

    /// Expression that names one concrete argument captured by a positional pack.
    struct expression_pack_arg
    {
        /// Source-level positional pack name to inspect.
        std::string pack_name;
        /// Constexpr expression that selects the zero-based pack element.
        expression index;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_pack_arg, pack_name, index);
    };

    struct expression_choose
    {
        expression condition;
        expression true_expr;
        expression false_expr;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_choose, condition, true_expr, false_expr);
    };

    struct delegate
    {
        // The name of the delegate
        std::string name;

        // TODO: Delegates should be able to refer to members by complex symbols

        // Expression arguments in a delegate call
        std::vector< expression_arg > args;

        RPNX_MEMBER_METADATA(delegate, name, args);
    };

    std::string to_string(type_symbol const&);

    std::optional< type_symbol > func_class(type_symbol const& func);

    inline std::optional< source_location > get_location(expression const& expr)
    {
        return rpnx::apply_visitor< std::optional< source_location > >(expr, [](auto const& value) { return value.location; });
    }

    inline void set_location(expression& expr, std::optional< source_location > location)
    {
        rpnx::apply_visitor<void>(expr, [&](auto& value) { value.location = location; });
    }
} // namespace quxlang

#endif // QUXLANG_DATA_BASIC_TYPES_HEADER_GUARD
