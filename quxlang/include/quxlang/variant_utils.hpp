// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VARIANT_UTILS_HEADER_GUARD
#define QUXLANG_VARIANT_UTILS_HEADER_GUARD



namespace quxlang
{


    template < typename T, typename... Ts >
    bool typeis(rpnx::variant< Ts... > const& v)
    {
        return v.template type_is< T>();
    }


    // Returns true if the variant currently holds any of the specified types
    template < typename... Us, typename... Ts >
    bool typeis_oneof(rpnx::variant< Ts... > const& v)
    {
        return (v.template type_is< Us >() || ...);
    }



    template < typename T, typename... Ts >
    T& as(rpnx::variant< Ts... >& v)
    {
        return v.template get_as< T >();
    }

    template < typename T, typename... Ts >
    T const& as(rpnx::variant< Ts... > const& v)
    {
        return v.template get_as< T >();
    }


} // namespace quxlang

#endif // QUXLANG_VARIANT_UTILS_HEADER_GUARD
