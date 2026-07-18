// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/lambda_types.hpp>
#include <quxlang/queries/specs/class_type_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::class_type_spec > quxlang::class_type_impl(type_symbol input)
{
    if (typeis< builtin_symbol >(input) && is_builtin_enum_name(as< builtin_symbol >(input).name))
    {
        co_return class_kind::enum_;
    }
    if (typeis< builtin_symbol >(input) && is_builtin_atomic_access_mode_name(as< builtin_symbol >(input).name))
    {
        co_return class_kind::primitive;
    }
    std::optional< type_symbol > const atomic_value_type = atomic_type_argument(input);
    if (atomic_value_type.has_value())
    {
        co_return is_valid_atomic_storage_type(*atomic_value_type) ? class_kind::primitive : class_kind::noexist;
    }

    if (parse_lambda_closure_symbol(input).has_value())
    {
        co_return class_kind::struct_;
    }

    if (typeis< readonly_constant >(input))
    {
        co_return class_kind::struct_;
    }

    if (typeis< numeric_literal_type >(input) || typeis< string_literal_type >(input) || typeis< bool_type >(input) || typeis< int_type >(input) ||
        typeis< float_type >(input) || typeis< procedure_type >(input) || typeis< ptrref_type >(input) || is_ref(input) || typeis< byte_type >(input) ||
        typeis< initguard_type >(input) || typeis< initguard_lock_type >(input) || typeis< constexpr_proxy >(input) || typeis< thistype >(input) ||
        typeis< address_type >(input) || typeis< size_type >(input) || typeis< array_type >(input) || typeis< attached_type_reference >(input) ||
        typeis< storage >(input) || typeis< aligned_storage >(input))
    {
        co_return class_kind::primitive;
    }

    if (typeis< subtag_type >(input))
    {
        std::optional< parameter_instantiation > const binding = co_await rpnx::querygraph::request< subtag_binding_query >(as< subtag_type >(input));
        if (binding.has_value() && binding->type_is< parameter_type_instantiation >())
        {
            co_return co_await rpnx::querygraph::request< class_type_query >(binding->get_as< parameter_type_instantiation >().type);
        }
        co_return class_kind::noexist;
    }

    ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (typeis< ast2_struct_declaration >(symboid))
    {
        co_return class_kind::struct_;
    }
    if (typeis< ast2_union_declaration >(symboid))
    {
        co_return class_kind::union_;
    }
    if (typeis< ast2_variant_declaration >(symboid))
    {
        co_return class_kind::variant;
    }
    if (typeis< ast2_enum_declaration >(symboid))
    {
        co_return class_kind::enum_;
    }
    if (typeis< ast2_flagset_declaration >(symboid))
    {
        co_return class_kind::flagset;
    }

    co_return class_kind::noexist;
}
