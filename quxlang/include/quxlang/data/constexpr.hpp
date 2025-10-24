// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_DATA_CONSTEXPR_HPP
#define QUXLANG_DATA_CONSTEXPR_HPP

#include <cstddef>
#include <quxlang/cow.hpp>
#include <quxlang/data/type_symbol.hpp>
#include <rpnx/metadata.hpp>
#include <vector>

namespace quxlang
{

    struct constexpr_result
    {
        cow< type_symbol > type;
        cow< std::vector< std::byte > > value;

        RPNX_MEMBER_METADATA(constexpr_result, type, value);
    };

} // namespace quxlang

#endif // QUXLANG_DATA_CONSTEXPR_HPP
