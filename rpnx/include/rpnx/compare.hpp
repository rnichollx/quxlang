// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef RPNX_COMPARE_HPP
#define RPNX_COMPARE_HPP
#include <compare>

namespace rpnx
{

    inline std::strong_ordering compare()
    {
        return std::strong_ordering::equal;
    }

    template <typename T>
    std::strong_ordering compare(T const& a, T const& b);
    template <typename T, typename T2, typename... Ts>
    std::strong_ordering compare(T const& a, T const& b, T2 const & t2, const Ts&... args);

    template <typename T, typename T2, typename... Ts>
    std::strong_ordering compare(T const& a, T const& b, T2 const & t2, const Ts&... args)
    {
        if (a < b)
            return std::strong_ordering::less;
        if (b < a)
            return std::strong_ordering::greater;
        return compare(t2, args...);
    }

    template <typename T>
    std::strong_ordering compare(T const& a, T const& b)
    {
        if (a < b)
            return std::strong_ordering::less;
        if (b < a)
            return std::strong_ordering::greater;
        return compare();
    }


}

#endif //COMPARE_HPP