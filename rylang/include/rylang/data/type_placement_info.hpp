//
// Created by Ryan Nicholl on 10/23/23.
//

#ifndef RYLANG_TYPE_PLACEMENT_INFO_HEADER_GUARD
#define RYLANG_TYPE_PLACEMENT_INFO_HEADER_GUARD

#include <cstddef>
namespace rylang
{
    struct type_placement_info
    {
        std::size_t size;
        std::size_t alignment;
    };
} // namespace rylang

#endif // RYLANG_TYPE_PLACEMENT_INFO_HEADER_GUARD
