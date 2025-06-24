// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_COW_HEADER_GUARD
#define QUXLANG_COW_HEADER_GUARD

#include <memory>
#include "rpnx/cow.hpp"

namespace quxlang
{
    /* Copy on write pointer */
    template < typename T >
    using cow = rpnx::cow< T >;
} // namespace quxlang

#endif // QUXLANG_COW_HEADER_GUARD
