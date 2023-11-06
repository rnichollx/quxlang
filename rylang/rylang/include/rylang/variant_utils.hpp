//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VARIANT_UTILS_HEADER
#define RPNX_RYANSCRIPT1031_VARIANT_UTILS_HEADER

#include <boost/type_index.hpp>
#include <boost/variant.hpp>

namespace rylang
{
    template < typename T, typename... Ts >
    bool typeis(boost::variant< Ts... > const& v)
    {
        return boost::typeindex::type_id< T >() == boost::typeindex::type_id_with_cvr< decltype(v) >();
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

#endif // RPNX_RYANSCRIPT1031_VARIANT_UTILS_HEADER
