// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_builtin_spec.hpp>

#include "quxlang/data/struct_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"
#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::class_builtin_spec > quxlang::class_builtin_impl(type_symbol input)
{
    type_symbol input_type = input;
    if (typeis< builtin_symbol >(input_type) && is_builtin_atomic_access_mode_name(as< builtin_symbol >(input_type).name))
    {
        co_return true;
    }
    if (is_atomic_type(input_type))
    {
        co_return true;
    }
    if (typeis< int_type >(input_type) || typeis< float_type >(input_type) || typeis< bool_type >(input_type) || typeis< procedure_type >(input_type) || typeis< ptrref_type >(input_type) || typeis< nvalue_slot >(input_type) || is_ref(input_type) || typeis<numeric_literal_type>(input) || typeis<numeric_literal_any_temploidic>(input) || typeis<string_literal_type>(input) || typeis<string_literal_any_temploidic>(input) || typeis<dvalue_slot>(input_type) || typeis<array_type>(input_type) || typeis< initguard_type >(input_type) || typeis< initguard_lock_type >(input_type) || typeis< constexpr_proxy >(input_type))
    {
        co_return true;
    }
    else
    {
        co_return false;
    }
}
