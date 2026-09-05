// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_STRUCT_INHERITANCE_HEADER_GUARD
#define QUXLANG_DATA_STRUCT_INHERITANCE_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/basic_types.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <rpnx/macros.hpp>

/** Identifies the runtime object category explicitly selected by a STRUCT declaration. */
RPNX_ENUM(quxlang, struct_polymorphism_kind, std::uint8_t, none, polymorphic, virtual_polymorphic);
/** Identifies whether inherited lookup selected an ordinary declaration or a base projection. */
RPNX_ENUM(quxlang, struct_member_candidate_kind, std::uint8_t, declaration, base_projection);
/** Describes the result of resolving one static struct conversion. */
RPNX_ENUM(quxlang, struct_conversion_status, std::uint8_t, unavailable, unique, ambiguous);
/** Selects forward base projection or unchecked recovery of the exact complete type. */
RPNX_ENUM(quxlang, inheritance_cast_direction, std::uint8_t, upcast, unchecked_static_downcast);
/** Selects the effective destructor behavior of a polymorphic struct. */
RPNX_ENUM(quxlang, struct_destructor_policy, std::uint8_t, category_default, nonvirtual);
/** Records how a full/subobject constructor pair entered normalized semantics. */
RPNX_ENUM(quxlang, constructor_form_origin, std::uint8_t, explicit_pair, constructor_template, compiler_generated);
/** Identifies an observable steady, construction, or destruction object phase. */
RPNX_ENUM(quxlang, struct_phase_kind, std::uint8_t, steady, construction, destruction);

namespace quxlang
{
    /** One active direct base after resolving its source type in the declaring struct context. */
    struct struct_base_declaration
    {
        /// Stable source selector; absent for the sole anonymous direct base form.
        std::optional< std::string > selector_name;
        /// Canonical instantiated STRUCT type selected by the declaration.
        type_symbol base_type;
        /// Embedding behavior of this direct edge.
        inheritance_kind kind = inheritance_kind::nonvirtual;
        /// Source-order ordinal among active direct base declarations.
        std::size_t declaration_ordinal = 0;
        /// Source location used by hierarchy diagnostics.
        std::optional< source_location > location;

        RPNX_MEMBER_METADATA(struct_base_declaration, selector_name, base_type, kind, declaration_ordinal, location);
    };

    /** Canonical identity of one subobject within a complete inheritance graph. */
    struct struct_subobject_id
    {
        /// Canonical virtual root type, or absent when the path begins at the complete object.
        std::optional< type_symbol > virtual_root;
        /// Direct-base ordinals below the complete object or canonical virtual root.
        std::vector< std::size_t > nonvirtual_path;

        RPNX_MEMBER_METADATA(struct_subobject_id, virtual_root, nonvirtual_path);
    };

    /** One direct edge in a statically selected path to an inherited subobject. */
    struct struct_subobject_path_step
    {
        /// Declaration ordinal in the step's source struct.
        std::size_t direct_base_ordinal = 0;
        /// Embedding behavior of the selected edge.
        inheritance_kind kind = inheritance_kind::nonvirtual;
        /// Canonical target type of the selected edge.
        type_symbol base_type;

        RPNX_MEMBER_METADATA(struct_subobject_path_step, direct_base_ordinal, kind, base_type);
    };

    /** One source-level inheritance path to a canonical subobject. */
    struct struct_subobject_path
    {
        std::vector< struct_subobject_path_step > steps;

        RPNX_MEMBER_METADATA(struct_subobject_path, steps);
    };

    /** Canonical subobject together with every source path that reaches it. */
    struct struct_subobject_record
    {
        struct_subobject_id id;
        type_symbol type;
        std::vector< struct_subobject_path > paths;

        RPNX_MEMBER_METADATA(struct_subobject_record, id, type, paths);
    };

    /** Normalized hierarchy graph shared by lookup, conversion, layout, lifecycle, and RTTI queries. */
    struct struct_inheritance_info
    {
        /// Canonical complete struct type described by this result.
        type_symbol complete_type;
        /// Active bases declared directly by the complete type.
        std::vector< struct_base_declaration > direct_bases;
        /// Complete object and every distinct nonvirtual or canonical virtual subobject.
        std::vector< struct_subobject_record > subobjects;
        /// Canonical virtual bases in semantic construction order.
        std::vector< type_symbol > virtual_base_order;
        /// Explicit runtime category of the complete type.
        struct_polymorphism_kind polymorphism = struct_polymorphism_kind::none;
        /// True when any reachable edge is virtual.
        bool has_virtual_inheritance = false;

        RPNX_MEMBER_METADATA(struct_inheritance_info, complete_type, direct_bases, subobjects, virtual_base_order, polymorphism, has_virtual_inheritance);
    };

    /** Canonical input for inherited declaration and base-selector lookup. */
    struct struct_member_lookup_input
    {
        type_symbol static_type;
        std::string member_name;

        RPNX_MEMBER_METADATA(struct_member_lookup_input, static_type, member_name);
    };

    /** One surviving inherited lookup candidate with its exact receiver subobject. */
    struct struct_member_lookup_candidate
    {
        struct_member_candidate_kind kind = struct_member_candidate_kind::declaration;
        /// Direct declaration symbol, or the source-owning pseudo-symbol of a base projection.
        type_symbol selected_declaration;
        /// Static type of the receiver after applying receiver_path.
        type_symbol receiver_type;
        struct_subobject_id receiver_subobject;
        struct_subobject_path receiver_path;

        RPNX_MEMBER_METADATA(struct_member_lookup_candidate, kind, selected_declaration, receiver_type, receiver_subobject, receiver_path);
    };

    /** Result of inherited lookup; more than one candidate is a diagnosed ambiguity. */
    struct struct_member_lookup_result
    {
        std::vector< struct_member_lookup_candidate > candidates;
        bool ambiguous = false;

        RPNX_MEMBER_METADATA(struct_member_lookup_result, candidates, ambiguous);
    };

    /** Canonical input for a type-directed derived-to-base conversion. */
    struct struct_conversion_input
    {
        type_symbol source_type;
        type_symbol destination_type;

        RPNX_MEMBER_METADATA(struct_conversion_input, source_type, destination_type);
    };

    /** Unique, unavailable, or ambiguous path set for a static inheritance conversion. */
    struct struct_conversion_result
    {
        struct_conversion_status status = struct_conversion_status::unavailable;
        std::optional< struct_subobject_path > path;
        std::vector< struct_subobject_path > candidate_paths;

        RPNX_MEMBER_METADATA(struct_conversion_result, status, path, candidate_paths);
    };

    /** Signature portion used to match a declaration against inherited virtual slots. */
    struct struct_virtual_signature
    {
        std::string name;
        std::vector< type_symbol > positional_parameters;
        std::map< std::string, type_symbol > named_parameters;
        type_symbol this_parameter;

        RPNX_MEMBER_METADATA(struct_virtual_signature, name, positional_parameters, named_parameters, this_parameter);
    };

    /** Stable identity of a virtual slot at the declaration that introduced it. */
    struct struct_virtual_slot_key
    {
        type_symbol introducing_declaration;
        struct_virtual_signature signature;

        RPNX_MEMBER_METADATA(struct_virtual_slot_key, introducing_declaration, signature);
    };

    /** Final routine selected when dispatch begins at one source subobject. */
    struct struct_virtual_overrider
    {
        struct_subobject_id source_subobject;
        std::vector< struct_subobject_path > source_paths;
        type_symbol final_overrider;
        bool is_final = false;
        bool is_pure = false;

        RPNX_MEMBER_METADATA(struct_virtual_overrider, source_subobject, source_paths, final_overrider, is_final, is_pure);
    };

    /** One normalized virtual slot and all source-subobject dispatch entries it requires. */
    struct struct_virtual_slot
    {
        struct_virtual_slot_key key;
        type_symbol return_type;
        std::vector< struct_virtual_overrider > overriders;
        std::size_t slot_ordinal = 0;

        RPNX_MEMBER_METADATA(struct_virtual_slot, key, return_type, overriders, slot_ordinal);
    };

    /** Deterministically ordered virtual slots and abstractness for one complete struct type. */
    struct struct_virtual_slots
    {
        std::vector< struct_virtual_slot > slots;
        bool is_abstract = false;
        struct_destructor_policy destructor_policy = struct_destructor_policy::category_default;

        RPNX_MEMBER_METADATA(struct_virtual_slots, slots, is_abstract, destructor_policy);
    };

    /** One normalized constructor signature and its distinct full/subobject declarations. */
    struct struct_constructor_form
    {
        temploid_ensig normalized_signature;
        type_symbol full_entry;
        type_symbol subobject_entry;
        constructor_form_origin origin = constructor_form_origin::compiler_generated;
        ast2_function_declaration full_declaration;
        ast2_function_declaration subobject_declaration;

        RPNX_MEMBER_METADATA(struct_constructor_form, normalized_signature, full_entry, subobject_entry, origin, full_declaration, subobject_declaration);
    };

    /** Constructor normalization result selected by the declared polymorphism category. */
    struct struct_constructor_forms
    {
        std::vector< struct_constructor_form > forms;
        bool uses_split_abi = false;

        RPNX_MEMBER_METADATA(struct_constructor_forms, forms, uses_split_abi);
    };

    /** Layout-independent runtime metadata requirements for one struct hierarchy. */
    struct struct_runtime_requirements
    {
        struct_polymorphism_kind polymorphism = struct_polymorphism_kind::none;
        struct_destructor_policy destructor_policy = struct_destructor_policy::category_default;
        std::vector< struct_subobject_id > runtime_header_subobjects;
        std::optional< struct_subobject_id > primary_base_candidate;
        bool requires_rtti = false;
        bool requires_virtual_base_navigation = false;
        bool effective_destructor_is_virtual = false;

        RPNX_MEMBER_METADATA(struct_runtime_requirements, polymorphism, destructor_policy, runtime_header_subobjects, primary_base_candidate, requires_rtti, requires_virtual_base_navigation, effective_destructor_is_virtual);
    };

    /** Identifies the active semantic type and subobject for one runtime phase. */
    struct struct_phase_key
    {
        struct_phase_kind kind = struct_phase_kind::steady;
        struct_subobject_id active_subobject;
        type_symbol active_type;

        RPNX_MEMBER_METADATA(struct_phase_key, kind, active_subobject, active_type);
    };

    /** Identifies one immutable descriptor within a complete type's phase groups. */
    struct struct_phase_descriptor_key
    {
        type_symbol complete_type;
        struct_phase_key phase;
        struct_subobject_id source_subobject;

        RPNX_MEMBER_METADATA(struct_phase_descriptor_key, complete_type, phase, source_subobject);
    };

    /** Assigns one runtime header when entering a child construction or destruction phase. */
    struct struct_phase_header_assignment
    {
        std::int64_t header_offset = 0;
        struct_phase_descriptor_key descriptor;

        RPNX_MEMBER_METADATA(struct_phase_header_assignment, header_offset, descriptor);
    };

    /** Ordered descriptor assignments for one fixed-index delegate transition. */
    struct struct_phase_transition
    {
        std::vector< struct_phase_header_assignment > header_assignments;

        RPNX_MEMBER_METADATA(struct_phase_transition, header_assignments);
    };

    /** Fixed-index transitions reachable from one active construction or destruction phase. */
    struct struct_phase_descriptor_group
    {
        struct_phase_key phase;
        std::vector< struct_phase_transition > field_transitions;
        std::vector< struct_phase_transition > direct_base_transitions;
        std::vector< struct_phase_transition > virtual_base_transitions;

        RPNX_MEMBER_METADATA(struct_phase_descriptor_group, phase, field_transitions, direct_base_transitions, virtual_base_transitions);
    };

    /** Concrete location of one canonical subobject in a complete runtime-bearing object. */
    struct struct_runtime_subobject
    {
        struct_subobject_id id;
        type_symbol type;
        std::int64_t offset = 0;
        bool has_runtime_header = false;

        RPNX_MEMBER_METADATA(struct_runtime_subobject, id, type, offset, has_runtime_header);
    };

    /** One target subobject considered by a dynamic cast from a runtime descriptor. */
    struct struct_runtime_cast_record
    {
        type_symbol target_type;
        struct_subobject_id target_subobject;
        std::int64_t target_offset = 0;

        RPNX_MEMBER_METADATA(struct_runtime_cast_record, target_type, target_subobject, target_offset);
    };

    /** One virtual dispatch entry and its statically determined receiver adjustment. */
    struct struct_adjustment_thunk
    {
        struct_virtual_slot_key slot;
        /** Ordinal in the static virtual interface of source_subobject. */
        std::size_t slot_ordinal = 0;
        struct_subobject_id source_subobject;
        struct_subobject_id target_subobject;
        type_symbol target_routine;
        std::int64_t receiver_adjustment = 0;

        RPNX_MEMBER_METADATA(struct_adjustment_thunk, slot, slot_ordinal, source_subobject, target_subobject, target_routine, receiver_adjustment);
    };

    /** Backend-ready hierarchy metadata after semantic slots and concrete layout are known. */
    struct struct_runtime_info
    {
        type_symbol complete_type;
        struct_runtime_requirements requirements;
        std::uint64_t allocation_size = 0;
        std::uint64_t allocation_align = 1;
        std::vector< struct_runtime_subobject > subobjects;
        std::vector< struct_runtime_cast_record > cast_records;
        std::vector< struct_phase_descriptor_group > descriptor_groups;
        std::vector< struct_adjustment_thunk > adjustment_thunks;

        RPNX_MEMBER_METADATA(struct_runtime_info, complete_type, requirements, allocation_size, allocation_align, subobjects, cast_records, descriptor_groups, adjustment_thunks);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_STRUCT_INHERITANCE_HEADER_GUARD
