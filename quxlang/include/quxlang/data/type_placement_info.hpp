//
// Created by Ryan Nicholl on 10/23/23.
//

#ifndef QUXLANG_TYPE_PLACEMENT_INFO_HEADER_GUARD
#define QUXLANG_TYPE_PLACEMENT_INFO_HEADER_GUARD

#include <cstddef>
namespace quxlang
{
    struct type_placement_info
    {
        std::size_t size;
        std::size_t alignment;
    };
} // namespace quxlang

#endif // QUXLANG_TYPE_PLACEMENT_INFO_HEADER_GUARD
