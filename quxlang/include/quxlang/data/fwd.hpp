//
// Created by Ryan Nicholl on 4/17/24.
//

#ifndef RPNX_QUXLANG_FWD_HEADER
#define RPNX_QUXLANG_FWD_HEADER

#include "rpnx/variant.hpp"
namespace quxlang
{
    struct module_reference;
    struct subentity_reference;
    struct subdotentity_reference;
    struct instanciation_reference;
    struct value_expression_reference;
    struct instance_pointer_type;
    struct primitive_type_integer_reference;
    struct primitive_type_bool_reference;
    struct mvalue_reference;
    struct tvalue_reference;
    struct ovalue_reference;
    struct cvalue_reference;
    struct avalue_reference;
    struct selection_reference;

    struct function_arg;
    struct function_overload;
    struct void_type;
    struct numeric_literal_reference;
    struct context_reference;
    struct template_reference;
    struct bound_function_type_reference;

    using type_symbol = rpnx::variant< void_type, context_reference, template_reference, module_reference, subentity_reference, primitive_type_integer_reference, primitive_type_bool_reference, instanciation_reference, selection_reference, value_expression_reference, subdotentity_reference, instance_pointer_type, tvalue_reference, mvalue_reference, cvalue_reference, ovalue_reference, bound_function_type_reference, numeric_literal_reference, avalue_reference >;

    struct expression_multiply;
    struct expression_modulus;
    struct expression_divide;
    struct expression_equals;
    struct expression_not_equals;
    struct expression_binary;
    struct target_expr;
    struct sizeof_expr;
    struct expression_this_reference;
    struct expression_call;
    struct expression_symbol_reference;
    struct expression_thisdot_reference;
    struct expression_dotreference;
    struct expression_numeric_literal;

    using expression = rpnx::variant< expression_this_reference, expression_call, expression_symbol_reference, expression_thisdot_reference, expression_dotreference, expression_binary, expression_numeric_literal, target_expr, sizeof_expr >;
}; // namespace quxlang

#endif // RPNX_QUXLANG_FWD_HEADER
