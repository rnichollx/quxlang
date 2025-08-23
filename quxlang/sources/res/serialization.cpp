// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#include "quxlang/compiler.hpp"
#include <quxlang/res/serialization.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_serialize_exists)
{
    auto serialize_symbol = submember{.of = input, .name = "SERIALIZE"};
    auto user_overloads = co_await QUX_CO_DEP(functum_user_overloads, (serialize_symbol));
    co_return !user_overloads.empty();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_deserialize_exists)
{
    auto deserialize_symbol = submember{.of = input, .name = "DESERIALIZE"};
    auto user_overloads = co_await QUX_CO_DEP(functum_user_overloads, (deserialize_symbol));
    co_return !user_overloads.empty();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(type_is_implicitly_datatype)
{
    // Check if the type is an int_type or bool_type
    if (typeis< int_type >(input) || typeis< bool_type >(input))
    {
        co_return true;
    }

    // Pointers and references are not implicitly datatypes
    if (typeis< ptrref_type >(input) )
    {
        co_return false;
    }

    auto type_kind = co_await QUX_CO_DEP(symbol_type, (input));

    if (type_kind == symbol_kind::class_)
    {


        auto class_fields = co_await QUX_CO_DEP(class_field_list, (input));
        for (const auto& field : class_fields)
        {
            // If any member is not a datatype, the class is not implicitly a datatype
            if (!(co_await QUX_CO_DEP(type_is_implicitly_datatype, (field.type))))
            {
                co_return false;
            }
        }
        co_return true;
    }

    // Default to false for other types
    co_return false;
}
