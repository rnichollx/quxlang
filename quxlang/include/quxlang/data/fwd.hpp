// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FWD_HEADER_GUARD
#define QUXLANG_DATA_FWD_HEADER_GUARD

#include "rpnx/variant.hpp"
namespace quxlang
{
    struct module_reference;
    struct subsymbol;
    struct submember;
    struct initialization_reference;
    struct instanciation_reference;
    struct value_expression_reference;
    struct pointer_type;
    struct int_type;
    struct bool_type;

    struct array_type;
    struct size_type;

    struct nvalue_slot;
    struct temploid_reference;

    struct function_arg;
    struct temploid_ensig;
    struct void_type;
    struct numeric_literal_reference;
    struct string_literal_reference;
    struct context_reference;
    struct auto_temploidic;
    struct dvalue_slot;
    struct bound_type_reference;
    struct thistype;
    struct pointer_type;
    struct type_temploidic;

    using type_symbol = rpnx::variant< void_type, context_reference, auto_temploidic, type_temploidic, module_reference, subsymbol, int_type, bool_type, initialization_reference, instanciation_reference, temploid_reference, value_expression_reference, submember, thistype, pointer_type, bound_type_reference, numeric_literal_reference, nvalue_slot, dvalue_slot, array_type, size_type >;

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

    using expression = rpnx::variant< expression_this_reference, expression_call, expression_symbol_reference, expression_thisdot_reference, expression_dotreference, expression_binary, expression_numeric_literal, expression_target, expression_sizeof, expression_string_literal, expression_rightarrow, expression_leftarrow, expression_multibind, expression_unary_postfix, expression_unary_prefix, expression_value_keyword >;

}; // namespace quxlang

#endif // RPNX_QUXLANG_FWD_HEADER
