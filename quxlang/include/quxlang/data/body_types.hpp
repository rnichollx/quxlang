// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_BODY_TYPES_HEADER_GUARD
#define QUXLANG_DATA_BODY_TYPES_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>

namespace quxlang
{
    /// One immutable publication of a function-local static object's state.
    struct publish_static_var
    {
        static_local_ref symbol;
        constexpr_static object;
        RPNX_MEMBER_METADATA(publish_static_var, symbol, object);
    };

    /// An immutable compile-time object or a local type binding.
    struct publish_static
    {
        rpnx::variant< type_symbol, publish_static_var > binding;
        RPNX_MEMBER_METADATA(publish_static, binding);
    };

    /// Declared and expression types of one runtime name.
    struct publish_decltype
    {
        type_symbol declared_type;
        type_symbol expression_type;
        RPNX_MEMBER_METADATA(publish_decltype, declared_type, expression_type);
    };

    /// Declaration information introduced or updated at one body level.
    using published_name = rpnx::variant< publish_static, publish_static_var, publish_decltype >;
}
#endif
