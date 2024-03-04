//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef QUXLANG_VARIANT_UTILS_HEADER_GUARD
#define QUXLANG_VARIANT_UTILS_HEADER_GUARD

#include <boost/type_index.hpp>
#include <boost/variant.hpp>

namespace quxlang
{
    template < typename T, typename... Ts >
    bool typeis(boost::variant< Ts... > const& v)
    {
        return v.type() == boost::typeindex::type_id< T >() ;
    }

    template < typename T, typename... Ts >
    bool typeis(rpnx::variant< Ts... > const& v)
    {
        return v.type() == boost::typeindex::type_id< T >() ;
    }

    template < typename T, typename... Ts >
    T& as(boost::variant< Ts... >& v)
    {
        return boost::get< T >(v);
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

    template < typename T, typename... Ts >
    T const& as(boost::variant< Ts... > const& v)
    {
        return boost::get< T >(v);
    }
} // namespace quxlang

#endif // QUXLANG_VARIANT_UTILS_HEADER_GUARD
