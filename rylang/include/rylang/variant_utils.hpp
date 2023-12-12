//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RYLANG_VARIANT_UTILS_HEADER_GUARD
#define RYLANG_VARIANT_UTILS_HEADER_GUARD

#include <boost/type_index.hpp>
#include <boost/variant.hpp>

namespace rylang
{
    template < typename T, typename... Ts >
    bool typeis(boost::variant< Ts... > const& v)
    {
        return v.type() == boost::typeindex::type_id< T >() ;
    }

    template < typename T, typename... Ts >
    T& as(boost::variant< Ts... >& v)
    {
        return boost::get< T >(v);
    }

    template < typename T, typename... Ts >
    T const& as(boost::variant< Ts... > const& v)
    {
        return boost::get< T >(v);
    }
} // namespace rylang

#endif // RYLANG_VARIANT_UTILS_HEADER_GUARD
