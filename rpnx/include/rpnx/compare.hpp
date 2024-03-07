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

    template <typename T, typename... Ts>
    std::strong_ordering compare(T const& a, T const& b, const Ts&... args)
    {
        if (a < b)
            return std::strong_ordering::less;
        if (b < a)
            return std::strong_ordering::greater;
        return compare(args...);
    }


}

#endif //COMPARE_HPP