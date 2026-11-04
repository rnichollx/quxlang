// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LOCAL_OPERANDS_HEADER_GUARD
#define QUXLANG_VMIR2_LOCAL_OPERANDS_HEADER_GUARD

#include <quxlang/vmir2/vmir2.hpp>

#include <string>
#include <tuple>
#include <type_traits>

namespace quxlang::vmir2
{
    /** Visits local operands in instruction metadata without traversing type or constant expression trees. */
    template < typename Value, typename Callback >
    void visit_local_operands(Value const& value, Callback const& callback)
    {
        using value_type = std::remove_cvref_t< Value >;
        if constexpr (std::is_same_v< value_type, local_index >)
        {
            callback(value);
        }
        else if constexpr (std::is_same_v< value_type, type_symbol > || std::is_same_v< value_type, expression > ||
                           std::is_same_v< value_type, antestatal_value > || std::is_same_v< value_type, source_location > ||
                           std::is_same_v< value_type, std::string >)
        {
        }
        else if constexpr (requires { value.template apply_visitor< void >([](auto&) {}); })
        {
            value.template apply_visitor< void >([&](auto& item) { visit_local_operands(item, callback); });
        }
        else if constexpr (requires { value.tie(); })
        {
            std::apply([&](auto&&... fields) { (visit_local_operands(fields, callback), ...); }, value.tie());
        }
        else if constexpr (requires { value.has_value(); *value; })
        {
            if (value.has_value()) visit_local_operands(*value, callback);
        }
        else if constexpr (requires { value.first; value.second; })
        {
            visit_local_operands(value.first, callback);
            visit_local_operands(value.second, callback);
        }
        else if constexpr (requires { value.begin(); value.end(); })
        {
            for (typename value_type::value_type const& item : value) visit_local_operands(item, callback);
        }
    }
}

#endif // QUXLANG_VMIR2_LOCAL_OPERANDS_HEADER_GUARD
