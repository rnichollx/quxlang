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
/// Selects whether an atomic-capable VMIR access is plain or atomic, and if atomic, its memory ordering.
RPNX_ENUM(quxlang, atomic_access_mode, std::uint8_t, nonatomic, atomic_relaxed, atomic_release, atomic_acquire, atomic_acqrel, atomic_seqcst);

RPNX_ENUM(quxlang, integral_qualifier, std::uint8_t, none, signed_, unsigned_);
RPNX_ENUM(quxlang, template_parameter_kind, std::uint8_t, type, value);
RPNX_ENUM(quxlang, lambda_capture_mode, std::uint8_t, reference, value);
RPNX_ENUM(quxlang, runtime_condition, std::uint16_t, CONSTEXPR, NATIVE);
/// Function-local compile-time storage class for STATIC and STATIC_VAR declarations.
RPNX_ENUM(quxlang, function_static_kind, std::uint16_t, constant, mutable_);
/// Describes how global variable storage becomes initialized before GET_REFERENCE returns it.
RPNX_ENUM(quxlang, initialization_type, std::uint8_t, init_with_guard, init_program_startup, init_trivial);

namespace quxlang
{
    std::optional< pointer_class > pointer_class_template_match(pointer_class template_class, pointer_class match_class);
    std::optional< qualifier > qualifier_template_match(qualifier to_qual, qualifier from_qual);

    std::optional< qualifier > qualifier_template_match_noconv(qualifier template_qual, qualifier match_qual);

    struct antestatal_struct;
    struct antestatal_array;
    struct antestatal_ptrref;
    struct antestatal_primitive;
    struct antestatal_interface;
    struct antestatal_fusion;
    struct antestatal_fusion_valueless;
    struct antestatal_fusion_active;

    using antestatal_value = rpnx::variant< antestatal_primitive, antestatal_array, antestatal_ptrref, antestatal_struct, antestatal_interface, antestatal_fusion >;

    struct antestatal_access_global;
    struct antestatal_access_field;
    struct antestatal_access_array_element;
    struct antestatal_access_fusion_payload;
    struct antestatal_nullptr;

    using antestatal_access = rpnx::variant< antestatal_nullptr, antestatal_access_global, antestatal_access_array_element, antestatal_access_field, antestatal_access_fusion_payload >;

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

    /** Identifies the payload storage of a particular fusion alternative. */
    struct antestatal_access_fusion_payload
    {
        antestatal_access fusion;
        std::uint64_t alternative = 0;

        RPNX_MEMBER_METADATA(antestatal_access_fusion_payload, fusion, alternative);
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

    /** Represents a valueless inline fusion. */
    struct antestatal_fusion_valueless
    {
        RPNX_EMPTY_METADATA(antestatal_fusion_valueless);
    };

    /** Represents an active inline fusion alternative and its optional non-VOID payload. */
    struct antestatal_fusion_active
    {
        std::uint64_t alternative = 0;
        std::optional< antestatal_value > payload;

        RPNX_MEMBER_METADATA(antestatal_fusion_active, alternative, payload);
    };

    /** Stores one exact semantic state of an inline fusion. */
    struct antestatal_fusion
    {
        rpnx::variant< antestatal_fusion_valueless, antestatal_fusion_active > state;

        RPNX_MEMBER_METADATA(antestatal_fusion, state);
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

    struct constexpr_string
    {
        std::vector< std::byte > bytes;

        RPNX_MEMBER_METADATA(constexpr_string, bytes);
    };

    struct constexpr_numeric
    {
        std::vector< std::byte > bytes;

        RPNX_MEMBER_METADATA(constexpr_numeric, bytes);
    };

    using constexpr_value = rpnx::variant< antestatal_value, constexpr_serialoid, constexpr_string, constexpr_numeric >;

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

    struct interface_slot_key
    {
        std::string name;
        invotype concrete_params;
        std::optional< type_symbol > concrete_return_type;

        RPNX_MEMBER_METADATA(interface_slot_key, name, concrete_params, concrete_return_type);
    };

    struct antestatal_interface
    {
        type_symbol interface_type;
        std::map< interface_slot_key, type_symbol > functions;
        bool is_default = false;

        RPNX_MEMBER_METADATA(antestatal_interface, interface_type, functions, is_default);
    };

    struct numeric_literal_type
    {
        std::string value;
        RPNX_MEMBER_METADATA(numeric_literal_type, value);
    };

    struct numeric_literal_any_temploidic
    {
        std::string name;
        RPNX_MEMBER_METADATA(numeric_literal_any_temploidic, name);
    };

    struct string_literal_type
    {
        std::string value;
        RPNX_MEMBER_METADATA(string_literal_type, value);
    };

    struct string_literal_any_temploidic
    {
        std::string name;
        RPNX_MEMBER_METADATA(string_literal_any_temploidic, name);
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

    struct decay_temploidic
    {
        std::string name;
        RPNX_MEMBER_METADATA(decay_temploidic, name);
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

    /// Template instantiation tag that exposes one instantiated template parameter as a symbol.
    /// This is an implementation detail, not a user facing type.
    struct subtag_type
    {
        /// Instantiated template or function that owns the parameter binding.
        type_symbol of;
        /// Source-level tag name exposed by the template parameter declaration.
        std::string name;

        RPNX_MEMBER_METADATA(subtag_type, of, name);
    };

    struct int_type
    {
        std::size_t bits = 0;
        bool has_sign = false;

        RPNX_MEMBER_METADATA(int_type, bits, has_sign);
    };

    struct float_type
    {
        std::size_t bits = 0;
        std::size_t exponent_bits = 0;

        RPNX_MEMBER_METADATA(float_type, bits, exponent_bits);
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

    /// ADDRESS is a unique pointer-valued type used by allocators to carry
    /// a raw machine address without any read/write authority. It is NOT an
    /// integer (unlike SZ) and lowers to a typeless (opaque) pointer in LLVM.
    struct address_type
    {
        RPNX_EMPTY_METADATA(address_type);
    };

    struct value_expression_reference
    {
        // TODO: Implement
        RPNX_EMPTY_METADATA(value_expression_reference);
    };



    /// Reference to a selected overload within a templexoid or functum.
    struct temploid_reference
    {
        /// Parent templexoid or functum that owns the selected overload.
        type_symbol templexoid;
        /// Optional zero-based overload identifier. Nullopt denotes the unique-overload form.
        std::optional< std::uint64_t > overload_id;

        RPNX_MEMBER_METADATA(temploid_reference, templexoid, overload_id);
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
        /**
         * Concrete owning type used when matching a formal THIS parameter that contains THISTYPE.
         * When this is VOID, THISTYPE is left as-is during ensig matching.
         */
        type_symbol type_of_this;

        RPNX_MEMBER_METADATA(ensig_initialization, ensig, params, adaptations, type_of_this);
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

    /// Reference to a selected overload instantiated with canonical formal template arguments.
    struct instanciation_reference
    {
        /// Selected overload reference for the instantiated templexoid or functum.
        temploid_reference temploid;
        /// Canonical formal template arguments for the selected overload.
        /// Synthesized member THIS parameters use THISTYPE here rather than the owning concrete class.
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

    struct decltype_type_ref
    {
        /// Symbol whose declared type should be resolved.
        type_symbol symbol;

        RPNX_MEMBER_METADATA(decltype_type_ref, symbol);
    };

    struct typeof_type_ref
    {
        /// Expression whose expression type should be resolved.
        expression expr;

        RPNX_MEMBER_METADATA(typeof_type_ref, expr);
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

    /// Returns true when a builtin name denotes a type-level atomic template.
    inline auto is_builtin_atomic_templex_name(std::string_view name) -> bool
    {
        return name == "ATOMIC";
    }

    /// Converts a builtin atomic access-mode class name into the corresponding VMIR mode.
    inline auto atomic_access_mode_from_name(std::string_view name) -> std::optional< atomic_access_mode >
    {
        if (name == "NONATOMIC")
        {
            return atomic_access_mode::nonatomic;
        }
        if (name == "ATOMIC_RELAXED")
        {
            return atomic_access_mode::atomic_relaxed;
        }
        if (name == "ATOMIC_RELEASE")
        {
            return atomic_access_mode::atomic_release;
        }
        if (name == "ATOMIC_ACQUIRE")
        {
            return atomic_access_mode::atomic_acquire;
        }
        if (name == "ATOMIC_ACQREL")
        {
            return atomic_access_mode::atomic_acqrel;
        }
        if (name == "ATOMIC_SEQCST")
        {
            return atomic_access_mode::atomic_seqcst;
        }
        return std::nullopt;
    }

    /// Returns true when a builtin name denotes an atomic access-mode class.
    inline auto is_builtin_atomic_access_mode_name(std::string_view name) -> bool
    {
        return atomic_access_mode_from_name(name).has_value();
    }

    /// Returns true when a builtin name denotes an atomic member operation.
    inline auto is_builtin_atomic_member_name(std::string_view name) -> bool
    {
        return name == "LOAD" || name == "STORE" || name == "COMPARE_EXCHANGE" || name == "FETCH_ADD" || name == "FETCH_SUB" || name == "FETCH_AND" || name == "FETCH_OR" || name == "FETCH_XOR" || name == "ADD" || name == "SUB" || name == "AND" || name == "OR" || name == "XOR";
    }

    /// Returns true when a builtin name denotes a nominal ENUM type.
    inline auto is_builtin_enum_name(std::string_view name) -> bool
    {
        return name == "ORDER";
    }

    /// Returns true when a builtin name is parsed as a type or type template.
    inline auto is_builtin_type_name(std::string_view name) -> bool
    {
        return is_builtin_atomic_templex_name(name) || is_builtin_atomic_access_mode_name(name) || is_builtin_enum_name(name);
    }

    inline auto is_builtin_global_functum_name(std::string_view name) -> bool
    {
        return name == "SERIALIZE_UINTANY" || name == "DESERIALIZE_UINTANY" || name == "SERIALIZE_LEB128" || name == "DESERIALIZE_LEB128" || name == "IEEE_EQUALS" || name == "IEEE_NOTEQUALS" || name == "IEEE_LESS" || name == "IEEE_GREATER";
    }

    /// Extracts the storage type parameter from a canonical ATOMIC#T type.
    auto atomic_type_argument(type_symbol const& type) -> std::optional< type_symbol >;

    /// Returns true when a type is permitted as ATOMIC#T storage.
    inline auto is_valid_atomic_storage_type(type_symbol const& type) -> bool
    {
        return type.template type_is< int_type >() || type.template type_is< byte_type >() || type.template type_is< bool_type >();
    }

    /// Returns true when the type is a canonical ATOMIC#T instantiation.
    inline auto is_atomic_type(type_symbol const& type) -> bool
    {
        return atomic_type_argument(type).has_value();
    }

    /// Returns the storage type for ATOMIC#T, otherwise returns the input type unchanged.
    inline auto atomic_storage_type_or_self(type_symbol const& type) -> type_symbol
    {
        if (auto value_type = atomic_type_argument(type); value_type.has_value())
        {
            return *value_type;
        }
        return type;
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
        std::vector< expression_arg > template_arguments;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_dotreference, lhs, field_name, template_arguments);
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

    struct lambda_capture
    {
        std::string name;
        lambda_capture_mode mode = lambda_capture_mode::reference;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(lambda_capture, name, mode);
    };

    struct ast2_function_parameter
    {
        /// Local source name for this parameter, or nullopt for an ignored parameter.
        std::optional< std::string > name;
        /// External call-site name for named parameters.
        std::optional< std::string > api_name;
        /// Declared parameter type or positional pack element type.
        type_symbol type;
        /// Optional default expression for omitted arguments.
        std::optional< expression > default_expr;
        /// True when this positional parameter is a variadic pack.
        bool is_pack = false;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_function_parameter, name, api_name, type, default_expr, is_pack);
    };

    struct function_expression_statement;
    struct function_if_statement;
    struct function_while_statement;
    struct function_for_statement;
    struct function_assert_statement;
    struct function_var_statement;
    struct function_unimplemented_statement;
    struct function_compilation_error_statement;
    struct function_panic_statement;
    struct function_place_statement;
    struct function_destroy_statement;
    struct function_runtime_statement;
    struct function_return_statement;
    struct function_static_eval_statement;
    struct function_static_if_statement;
    struct function_static_while_statement;
    struct function_break_statement;
    struct function_continue_statement;
    struct function_label_statement;
    struct function_label_block_statement;
    struct function_goto_statement;
    struct function_match_statement;

    using function_statement = rpnx::variant< function_block, function_expression_statement, function_if_statement, function_while_statement, function_for_statement, function_var_statement, function_return_statement, function_assert_statement, function_unimplemented_statement, function_compilation_error_statement, function_panic_statement, function_place_statement, function_destroy_statement, function_runtime_statement, function_static_eval_statement, function_static_if_statement, function_static_while_statement, function_break_statement, function_continue_statement, function_label_statement, function_label_block_statement, function_goto_statement, function_match_statement >;

    struct function_var_statement
    {
        std::string name;
        type_symbol type;
        std::vector< expression_arg > initializers;

        std::optional< expression > equals_initializer;
        /// Storage class for function-local STATIC/STATIC_VAR declarations; null for VAR.
        std::optional< function_static_kind > static_kind;

        QUX_AST_METADATA(function_var_statement, name, type, initializers, equals_initializer, static_kind);
    };

    struct function_unimplemented_statement
    {
        std::optional< std::string > error_message;

        QUX_AST_METADATA(function_unimplemented_statement, error_message);
    };

    /// Statement that rejects VMIR generation when the statement is reached.
    struct function_compilation_error_statement
    {
        std::optional< std::string > message;
        /// Defers the diagnostic until the containing VMIR path is consumed.
        bool on_lower = false;

        QUX_AST_METADATA(function_compilation_error_statement, message, on_lower);
    };

    /** Unconditionally terminates execution with an optional diagnostic message. */
    struct function_panic_statement
    {
        std::optional< std::string > message;

        QUX_AST_METADATA(function_panic_statement, message);
    };

    struct function_block
    {
        std::vector< function_statement > statements;
        std::string block_dbg_string;

        QUX_AST_METADATA(function_block, statements, block_dbg_string);
    };

    /** Selects one named UNION alternative in a MATCH arm. */
    struct union_match_selector
    {
        std::string option_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(union_match_selector, option_name);
    };

    /** Selects one canonical VARIANT alternative type in a MATCH arm. */
    struct variant_match_selector
    {
        type_symbol type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(variant_match_selector, type);
    };

    using match_selector = rpnx::variant< union_match_selector, variant_match_selector >;

    /** One CASE or TYPE arm in a MATCH statement. */
    struct function_match_arm
    {
        match_selector selector;
        std::optional< std::string > binding_name;
        std::optional< expression > where_condition;
        bool otherwise = false;
        function_block block;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(function_match_arm, selector, binding_name, where_condition, otherwise, block);
    };

    /** The final DEFAULT block or DEFAULT FAIL clause of a MATCH statement. */
    struct function_match_default
    {
        bool fail = false;
        std::optional< function_block > block;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(function_match_default, fail, block);
    };

    /** Runtime alternative dispatch over a UNION or VARIANT value. */
    struct function_match_statement
    {
        expression subject;
        std::optional< std::string > binding_name;
        bool shadow = false;
        std::vector< function_match_arm > arms;
        std::optional< function_match_default > default_clause;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(function_match_statement, subject, binding_name, shadow, arms, default_clause);
    };

    struct function_assert_statement
    {
        expression condition;
        std::string expr_text;
        std::optional< std::string > tagline;

        QUX_AST_METADATA(function_assert_statement, condition, expr_text, tagline);
    };

    struct function_runtime_statement
    {
        runtime_condition condition;
        function_block then_block;
        std::optional< function_block > else_block;

        QUX_AST_METADATA(function_runtime_statement, condition, then_block, else_block);
    };

    struct function_expression_statement
    {
        expression expr;

        QUX_AST_METADATA(function_expression_statement, expr);
    };

    struct function_static_eval_statement
    {
        /// Expression evaluated immediately during VMIR generation.
        expression expr;

        QUX_AST_METADATA(function_static_eval_statement, expr);
    };

    struct function_if_statement
    {
        expression condition;
        function_block then_block;
        std::optional< function_block > else_block;

        QUX_AST_METADATA(function_if_statement, condition, then_block, else_block);
    };

    struct function_static_if_statement
    {
        /// Compile-time condition evaluated before choosing which block to generate.
        expression condition;
        /// Block generated when condition evaluates to true.
        function_block then_block;
        /// Optional STATIC_ELSE block generated when condition evaluates to false.
        std::optional< function_block > else_block;

        QUX_AST_METADATA(function_static_if_statement, condition, then_block, else_block);
    };

    struct function_while_statement
    {
        std::optional< std::string > label_name;
        expression condition;
        function_block loop_block;

        QUX_AST_METADATA(function_while_statement, label_name, condition, loop_block);
    };

    struct function_for_statement
    {
        std::optional< std::string > label_name;

        std::optional< function_block > init_block;
        std::optional< function_block > eval_block;
        std::optional< expression > test_condition;
        std::optional< expression > posttest_condition;
        std::optional< function_block > step_block;

        std::optional< std::string > iter_name;
        std::optional< std::string > value_name;
        std::optional< std::string > index_name;
        std::optional< std::string > item_name;

        std::optional< expression > in_expr;
        std::optional< expression > start_expr;
        std::optional< expression > end_expr;
        std::optional< expression > limit_expr;
        std::optional< expression > filter_expr;
        std::optional< expression > by_expr;
        std::optional< expression > from_expr;
        std::optional< expression > to_expr;
        std::optional< expression > until_expr;

        function_block loop_block;

        QUX_AST_METADATA(function_for_statement, label_name, init_block, eval_block, test_condition, posttest_condition, step_block, iter_name, value_name, index_name, item_name, in_expr, start_expr, end_expr, limit_expr, filter_expr, by_expr, from_expr, to_expr, until_expr, loop_block);
    };

    struct function_static_while_statement
    {
        /// Compile-time condition re-evaluated before each generated iteration.
        expression condition;
        /// Block generated once per true compile-time condition result.
        function_block loop_block;

        QUX_AST_METADATA(function_static_while_statement, condition, loop_block);
    };

    struct function_break_statement
    {
        std::optional< std::string > label_name;

        QUX_AST_METADATA(function_break_statement, label_name);
    };

    struct function_continue_statement
    {
        std::optional< std::string > label_name;

        QUX_AST_METADATA(function_continue_statement, label_name);
    };

    struct function_label_statement
    {
        std::string name;

        QUX_AST_METADATA(function_label_statement, name);
    };

    struct function_label_block_statement
    {
        std::string name;
        function_block block;

        QUX_AST_METADATA(function_label_block_statement, name, block);
    };

    struct function_goto_statement
    {
        std::string target;

        QUX_AST_METADATA(function_goto_statement, target);
    };

    struct function_return_statement
    {
        std::optional< expression > expr;

        QUX_AST_METADATA(function_return_statement, expr);
    };

    struct function_place_statement
    {
        // The expression which yields a pointer to the location to place the object.
        expression at;

        // Type to place.
        type_symbol type;

        // Optional assignment initializer,
        // If present, args must be empty.
        // e.g. PLACE AT(loc) type := assign_init_expr;
        std::optional< expression > assign_init;

        // Optional constructor args,
        // e.g. PLACE AT(loc) type :(args...);
        std::vector< expression_arg > args;

        QUX_AST_METADATA(function_place_statement, at, type, assign_init, args);
    };

    struct function_destroy_statement
    {
        // The expression which yields the storage location to destroy the object within.
        expression at;
        // Type to destroy
        type_symbol type;
        // Optional destructor args.
        std::vector< expression_arg > args;

        QUX_AST_METADATA(function_destroy_statement, at, type, args);
    };


    struct expression_lambda
    {
        std::vector< lambda_capture > captures;
        bool has_explicit_capture_list = false;
        std::vector< ast2_function_parameter > parameters;
        std::optional< type_symbol > return_type;
        function_block body;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_lambda, captures, has_explicit_capture_list, parameters, return_type, body);
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

    struct expression_numeric_literal_fits
    {
        type_symbol literal_type;
        type_symbol target_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_numeric_literal_fits, literal_type, target_type);
    };

    struct expression_numeric_literal_binary_op
    {
        std::string op;
        type_symbol lhs_type;
        type_symbol rhs_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_numeric_literal_binary_op, op, lhs_type, rhs_type);
    };

    struct expression_numeric_literal_negate
    {
        type_symbol operand_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_numeric_literal_negate, operand_type);
    };

    struct expression_typecast
    {
        expression expr;
        type_symbol to_type;
        std::optional< std::string > keyword;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_typecast, expr, to_type, keyword);
    };

    /** Tests whether a UNION contains one named alternative. */
    struct expression_union_is
    {
        expression subject;
        std::string option_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_union_is, subject, option_name);
    };

    /** Tests whether a VARIANT contains one alternative type. */
    struct expression_variant_isa
    {
        expression subject;
        type_symbol type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_variant_isa, subject, type);
    };

    /** Returns a qualified reference to an active non-VOID VARIANT payload. */
    struct expression_variant_unwrap
    {
        expression subject;
        type_symbol type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_variant_unwrap, subject, type);
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

    struct expression_forward
    {
        /// Symbol to forward with its declared reference type.
        type_symbol symbol;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_forward, symbol);
    };

    struct expression_choose
    {
        expression condition;
        expression true_expr;
        expression false_expr;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_choose, condition, true_expr, false_expr);
    };

    /// `ADDRESS_LAUNDER <expr> TO <type>` -- converts an ADDRESS to a pointer without
    /// changing its provenance. Address laundering is not permitted during constexpr evaluation.
    struct expression_address_launder
    {
        expression address;
        type_symbol to_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_address_launder, address, to_type);
    };

    /// `ADDRESS_LAUNDER_FROM <expr>` -- converts a pointer to ADDRESS without changing
    /// its provenance. Address laundering is not permitted during constexpr evaluation.
    struct expression_address_launder_from
    {
        expression pointer;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_address_launder_from, pointer);
    };

    // Provenance-related alloc region keyword expressions (see docs/disorganized_ideas/provenance.md).
    // For now these are parsed and carried through the AST; in the VMIR/LLVM backends they lower
    // like REINTERPRET casts (no provenance tracking or LLVM intrinsics yet).

    /// `BEGIN_ALLOC_REGION <expr> AS <type>` -- casts an ADDRESS to a storage pointer while
    /// (eventually) beginning storage provenance in the selected region.
    struct expression_begin_alloc_region
    {
        expression address;
        type_symbol as_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_begin_alloc_region, address, as_type);
    };

    /// `END_ALLOC_REGION <storage_pointer>` -- ends an alloc region allocated with
    /// BEGIN_ALLOC_REGION and returns an ADDRESS with the parent provenance.
    struct expression_end_alloc_region
    {
        expression pointer;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_end_alloc_region, pointer);
    };

    /// `BEGIN_MULTI_ALLOC_REGION <expr> SIZE <count> AS <type>` -- like BEGIN_ALLOC_REGION but
    /// includes a count of storage elements, for dynamically sized arrays of known storage types.
    struct expression_begin_multi_alloc_region
    {
        expression address;
        expression count;
        type_symbol as_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_begin_multi_alloc_region, address, count, as_type);
    };

    /// `END_MULTI_ALLOC_REGION <expr> [SIZE <count>]` -- pops the provenance region of a
    /// multi-alloc and returns an ADDRESS.
    struct expression_end_multi_alloc_region
    {
        expression pointer;
        std::optional< expression > count;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_end_multi_alloc_region, pointer, count);
    };

    /// `RESIZE_MULTI_ALLOC_REGION <expr> COUNT <newcount>` -- changes the alloc size of an
    /// existing multi-alloc.
    struct expression_resize_multi_alloc_region
    {
        expression pointer;
        expression newcount;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_resize_multi_alloc_region, pointer, newcount);
    };

    /// `BEGIN_DYNAMIC_ALLOC_REGION <expr> SIZE <count>` -- takes an ADDRESS and returns a
    /// provenance-constrained ADDRESS.
    struct expression_begin_dynamic_alloc_region
    {
        expression address;
        expression count;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_begin_dynamic_alloc_region, address, count);
    };

    /// `END_DYNAMIC_ALLOC_REGION <expr> SIZE <count>` -- recovers a pointer to the parent
    /// provenance while ending the provenance constraint of the provided address.
    struct expression_end_dynamic_alloc_region
    {
        expression address;
        expression count;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_end_dynamic_alloc_region, address, count);
    };

    /// `RESIZE_DYNAMIC_ALLOC_REGION <expr> SIZE <newsize>` -- changes the alloc size of an
    /// existing dynamic alloc region in-place.
    struct expression_resize_dynamic_alloc_region
    {
        expression address;
        expression newsize;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_resize_dynamic_alloc_region, address, newsize);
    };

    /// `PARENT_ALLOC_ADDRESS <storage_pointer or address>` -- exposes the address of the
    /// allocation associated only with the direct parent allocation.
    struct expression_parent_alloc_address
    {
        expression pointer_or_address;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_parent_alloc_address, pointer_or_address);
    };

    /// `RELOCATE_REGION_OBJECTS FROM <alloc1> TO <alloc2> SIZE <byte count>` -- relocates any
    /// live objects in the source region to the destination region.
    struct expression_relocate_region_objects
    {
        expression from;
        expression to;
        expression byte_count;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_relocate_region_objects, from, to, byte_count);
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

    inline std::optional< source_location > get_location(function_statement const& st)
    {
        return rpnx::apply_visitor< std::optional< source_location > >(st, [](auto const& value) { return value.location; });
    }
} // namespace quxlang

#endif // QUXLANG_DATA_BASIC_TYPES_HEADER_GUARD
