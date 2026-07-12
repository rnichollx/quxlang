// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FWD_HEADER_GUARD
#define QUXLANG_DATA_FWD_HEADER_GUARD

#include "rpnx/variant.hpp"

namespace quxlang
{
    struct absolute_module_reference;
    struct subsymbol;
    struct submember;
    struct subtag_type;
    struct initialization_reference;
    struct instanciation_reference;
    struct value_expression_reference;
    struct procedure_type;
    struct ptrref_type;
    struct int_type;
    struct float_type;
    struct bool_type;
    struct readonly_constant;

    struct array_type;
    struct size_type;
    struct address_type;

    struct nvalue_slot;
    struct temploid_reference;

    struct function_arg;
    struct temploid_ensig;
    struct void_type;
    struct numeric_literal_type;
    struct numeric_literal_any_temploidic;
    struct string_literal_type;
    struct string_literal_any_temploidic;
    struct context_reference;
    struct auto_temploidic;
    struct decay_temploidic;
    struct dvalue_slot;
    struct attached_type_reference;
    struct thistype;
    struct ptrref_type;
    struct type_temploidic;
    struct freebound_identifier;
    struct builtin_symbol;
    struct byte_type;
    struct initguard_type;
    struct initguard_lock_type;
    struct constexpr_proxy;
    struct storage;
    struct aligned_storage;
    struct array_initializer_type;
    struct static_local_ref;
    struct static_snapshot_ref;
    struct pack_arg_type_ref;
    struct decltype_type_ref;
    struct typeof_type_ref;
    struct function_block;

    using type_symbol = rpnx::variant< void_type, byte_type, initguard_type, initguard_lock_type, constexpr_proxy, freebound_identifier, builtin_symbol, context_reference, auto_temploidic, decay_temploidic, type_temploidic, absolute_module_reference, subsymbol, subtag_type, int_type, float_type, bool_type, initialization_reference, instanciation_reference, temploid_reference, value_expression_reference, submember, thistype, procedure_type, ptrref_type, attached_type_reference, numeric_literal_type, numeric_literal_any_temploidic, string_literal_type, string_literal_any_temploidic, nvalue_slot, dvalue_slot, array_type, size_type, address_type, readonly_constant, storage, aligned_storage, array_initializer_type, static_local_ref, static_snapshot_ref, pack_arg_type_ref, decltype_type_ref, typeof_type_ref >;

    struct expression_multiply;
    struct expression_modulus;
    struct expression_divide;
    struct expression_equals;
    struct expression_not_equals;
    struct expression_binary;
    struct expression_target;
    struct expression_sizeof;
    struct expression_this_reference;
    struct expression_call;
    struct expression_symbol_reference;
    struct expression_thisdot_reference;
    struct expression_dotreference;
    struct expression_numeric_literal;
    struct expression_string_literal;
    struct expression_rightarrow;
    struct expression_leftarrow;
    struct expression_multibind;
    struct expression_unary_postfix;
    struct expression_unary_prefix;
    struct expression_value_keyword;
    struct absolute_module_reference;
    struct expression_char_literal;

    struct expression_sizeof;
    struct expression_bits;
    struct expression_is_signed;
    struct expression_numeric_literal_fits;
    struct expression_numeric_literal_binary_op;
    struct expression_numeric_literal_negate;
    struct expression_is_integral;
    struct expression_same_types;
    struct expression_typecast;
    struct expression_pun;
    struct expression_place;

    struct expression_static_choose;
    struct expression_choose;
    struct expression_begin_alloc_region;
    struct expression_end_alloc_region;
    struct expression_begin_multi_alloc_region;
    struct expression_end_multi_alloc_region;
    struct expression_resize_multi_alloc_region;
    struct expression_begin_dynamic_alloc_region;
    struct expression_end_dynamic_alloc_region;
    struct expression_resize_dynamic_alloc_region;
    struct expression_parent_alloc_address;
    struct expression_relocate_region_objects;
    struct expression_snapshot;
    struct expression_pack_size;
    struct expression_pack_arg;
    struct expression_forward;
    struct expression_lambda;
    struct expression_union_is;
    struct expression_variant_isa;
    struct expression_variant_unwrap;

    using expression = rpnx::variant< expression_symbol_reference, expression_this_reference, expression_call, expression_thisdot_reference, expression_dotreference, expression_binary, expression_numeric_literal, expression_target, expression_sizeof, expression_string_literal, expression_rightarrow, expression_leftarrow, expression_multibind, expression_unary_postfix, expression_unary_prefix, expression_value_keyword, expression_char_literal, expression_sizeof, expression_bits, expression_is_signed, expression_is_integral, expression_same_types, expression_numeric_literal_fits, expression_numeric_literal_binary_op, expression_numeric_literal_negate, expression_typecast, expression_pun, expression_place, expression_choose, expression_static_choose, expression_snapshot, expression_pack_size, expression_pack_arg, expression_forward, expression_lambda, expression_union_is, expression_variant_isa, expression_variant_unwrap, expression_begin_alloc_region, expression_end_alloc_region, expression_begin_multi_alloc_region, expression_end_multi_alloc_region, expression_resize_multi_alloc_region, expression_begin_dynamic_alloc_region, expression_end_dynamic_alloc_region, expression_resize_dynamic_alloc_region, expression_parent_alloc_address, expression_relocate_region_objects >;

    struct call_initializer;
    struct array_initializer;
    struct assignment_initializer;

    using initializer  = rpnx::variant< assignment_initializer, call_initializer, array_initializer >;

}; // namespace quxlang

extern template quxlang::type_symbol::basic_variant(quxlang::type_symbol::allocator_type const&);
extern template quxlang::type_symbol::basic_variant(quxlang::type_symbol const&);
extern template quxlang::expression::basic_variant(quxlang::expression::allocator_type const&);
extern template quxlang::expression::basic_variant(quxlang::expression const&);

#endif // RPNX_QUXLANG_FWD_HEADER
