// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CONSTEXPR_HEADER_GUARD
#define QUXLANG_DATA_CONSTEXPR_HEADER_GUARD

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
