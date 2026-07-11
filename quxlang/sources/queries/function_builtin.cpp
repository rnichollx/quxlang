// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_builtin_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"
#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::function_builtin_spec > quxlang::function_builtin_impl(temploid_reference input)
{
    auto const& user_overloads = co_await rpnx::querygraph::request< functum_map_user_formal_ensigs_query >(input.templexoid);
    auto builtin_overloads = co_await rpnx::querygraph::request< functum_builtin_overloads_query >(input.templexoid);
    auto const total_count = user_overloads.size() + builtin_overloads.size();

    if (!input.overload_id.has_value())
    {
        if (!(user_overloads.empty() && total_count == 1))
        {
            co_return builtin_function_kind::not_builtin;
        }
    }
    else if (*input.overload_id < user_overloads.size())
    {
        co_return builtin_function_kind::not_builtin;
    }
    else if (*input.overload_id >= total_count)
    {
        co_return builtin_function_kind::not_builtin;
    }

    auto classify_builtin_symbol = [](builtin_symbol const& builtin) -> std::optional< builtin_function_kind >
    {
        if (builtin_allocator_kind_from_name(builtin.name).has_value())
        {
            return builtin_function_kind::builtin_intrinsic;
        }
        if (builtin.name == "IEEE_EQUALS" || builtin.name == "IEEE_NOTEQUALS" || builtin.name == "IEEE_LESS" || builtin.name == "IEEE_GREATER")
        {
            return builtin_function_kind::builtin_intrinsic;
        }
        if (builtin.name == "SERIALIZE_UINTANY" || builtin.name == "DESERIALIZE_UINTANY" || builtin.name == "SERIALIZE_LEB128" || builtin.name == "DESERIALIZE_LEB128")
        {
            return builtin_function_kind::builtin_generated_routine;
        }
        return std::nullopt;
    };
    auto is_intrinsic_builtin_type = [](type_symbol const& type) -> bool
    {
        return type.type_is< int_type >() || type.type_is< float_type >() || type.type_is< bool_type >() || type.type_is< procedure_type >() || type.type_is< ptrref_type >() || type.type_is< array_type >() || type.type_is< byte_type >() || type.type_is< readonly_constant >() || type.type_is< constexpr_proxy >() || type.type_is< address_type >() || is_atomic_type(type);
    };

    type_symbol classified_symbol = input.templexoid;
    if (typeis< instanciation_reference >(classified_symbol))
    {
        classified_symbol = as< instanciation_reference >(classified_symbol).temploid.templexoid;
    }

    if (typeis< builtin_symbol >(classified_symbol))
    {
        builtin_symbol const& builtin = as< builtin_symbol >(classified_symbol);
        if (std::optional< builtin_function_kind > result = classify_builtin_symbol(builtin); result.has_value())
        {
            co_return *result;
        }
        throw compiler_bug("Missing builtin classification for builtin symbol: " + builtin.name);
    }

    if (typeis< subsymbol >(classified_symbol))
    {
        subsymbol const& ss = as< subsymbol >(classified_symbol);
        if (ss.name == "GET_INTERFACE_IMPL" && co_await rpnx::querygraph::request< symbol_type_query >(ss.of) == symbol_kind::implementation_)
        {
            co_return builtin_function_kind::builtin_special;
        }
        throw compiler_bug("Missing builtin classification for subsymbol: " + to_string(input));
    }

    if (!typeis< submember >(classified_symbol))
    {
        throw compiler_bug("Missing builtin classification for non-member builtin function: " + to_string(input));
    }

    submember const& member = as< submember >(classified_symbol);
    if (typeis< initguard_type >(member.of))
    {
        if (member.name == "LOAD" || member.name == "STORE" || member.name == "COMPARE_EXCHANGE")
        {
            co_return builtin_function_kind::builtin_intrinsic;
        }
        co_return builtin_function_kind::not_builtin;
    }

    symbol_kind const parent_kind = co_await rpnx::querygraph::request< symbol_type_query >(member.of);
    class_kind const parent_class_kind = parent_kind == symbol_kind::class_
                                             ? co_await rpnx::querygraph::request< class_type_query >(member.of)
                                             : class_kind::noexist;
    if (parent_kind == symbol_kind::interface_ || parent_kind == symbol_kind::global_variable || parent_class_kind == class_kind::enum_ || parent_class_kind == class_kind::flagset)
    {
        co_return builtin_function_kind::builtin_special;
    }
    if (typeis< constexpr_proxy >(member.of))
    {
        co_return builtin_function_kind::builtin_intrinsic;
    }
    if (member.name == "CONSTRUCTOR" && typeis< array_type >(member.of))
    {
        co_return builtin_function_kind::builtin_special;
    }
    if (member.name == "BEGIN" || member.name == "END" || member.name == "SERIALIZE" || member.name == "DESERIALIZE")
    {
        co_return builtin_function_kind::builtin_generated_routine;
    }
    if (is_intrinsic_builtin_type(member.of))
    {
        co_return builtin_function_kind::builtin_intrinsic;
    }
    if (member.name == "CONSTRUCTOR" || member.name == "DESTRUCTOR" || member.name == "OPERATOR<->" || member.name == "OPERATOR==" || member.name == "OPERATOR!=" || member.name == "OPERATOR:=")
    {
        co_return builtin_function_kind::builtin_generated_routine;
    }

    throw compiler_bug("Missing builtin classification for: " + to_string(input));
}
