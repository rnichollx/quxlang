// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_TYPE_SYMBOL_HEADER_GUARD
#define QUXLANG_DATA_TYPE_SYMBOL_HEADER_GUARD

#include "rpnx/variant.hpp"
#include <compare>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <rpnx/compare.hpp>
#include <rpnx/macros.hpp>


#include "quxlang/data/symbol_type.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/data/fwd.hpp>

RPNX_ENUM(quxlang, overload_class, std::uint16_t, user_defined, builtin, intrinsic);

RPNX_ENUM(quxlang, qualifier, std::uint16_t, mut, constant, temp, write, auto_, input, output);
RPNX_ENUM(quxlang, pointer_class, std::uint16_t, instance, array, machine, ref);

RPNX_ENUM(quxlang, constant_kind, std::uint16_t, data, numeric, string, cstring);
RPNX_ENUM(quxlang, allowed_adaptations, std::uint8_t, source_rebinding, class_conversions, destination_rebinding, none);
RPNX_ENUM(quxlang, conversion_type, std::uint8_t, implicit, explicit_, partial, assume, checked);

namespace quxlang
{
    std::optional< pointer_class > pointer_class_template_match(pointer_class template_class, pointer_class match_class);
    std::optional< qualifier > qualifier_template_match(qualifier to_qual, qualifier from_qual);

    std::optional< qualifier > qualifier_template_match_noconv(qualifier template_qual, qualifier match_qual);

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
        type_symbol type;
        std::optional< expression > default_value;

        RPNX_MEMBER_METADATA(declared_parameter, api_name, name, type, default_value);
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
        type_symbol type;
        std::optional< expression > default_value;

        RPNX_MEMBER_METADATA(parameter_type, type, default_value);
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
        type_symbol type;
        bool is_defaulted = false;

        RPNX_MEMBER_METADATA(argif, type, is_defaulted);
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

    struct array_type
    {
        type_symbol element_type;
        expression element_count;

        RPNX_MEMBER_METADATA(array_type, element_type, element_count);
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
    // struct function_type_reference;

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

    struct initialization_reference
    {
        type_symbol initializee;
        invotype parameters;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(initialization_reference, initializee, parameters, adaptations);
    };

    struct temploid_reference
    {
        type_symbol templexoid;
        temploid_ensig which;
        RPNX_MEMBER_METADATA(temploid_reference, templexoid, which);
    };

    struct ensig_initialization
    {
        temploid_ensig ensig;
        invotype params;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(ensig_initialization, ensig, params, adaptations);
    };

    struct instanciation_reference
    {
        temploid_reference temploid;
        invotype params;
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

    struct keyword_symbol
    {
        std::string name;

        RPNX_MEMBER_METADATA(keyword_symbol, name);
    };

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

    std::string to_string(type_symbol const&);

    std::optional< type_symbol > func_class(type_symbol const& func);

} // namespace quxlang

#include <quxlang/data/expression.hpp>

#endif // QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD
