// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_builtin_spec.hpp>

#include "quxlang/data/class_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"


rpnx::querygraph::coroutine< quxlang::class_builtin_spec > quxlang::class_builtin_impl(type_symbol input)
{
    type_symbol input_type = input;
    if (typeis< int_type >(input_type) || typeis< bool_type >(input_type) || typeis< procedure_type >(input_type) || typeis< ptrref_type >(input_type) || typeis< nvalue_slot >(input_type) || is_ref(input_type) || typeis<numeric_literal_reference>(input) || typeis<dvalue_slot>(input_type) || typeis<array_type>(input_type) || typeis< initguard_type >(input_type) || typeis< initguard_lock_type >(input_type))
    {
        co_return true;
    }
    else
    {
        co_return false;
    }
}
