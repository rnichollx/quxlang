//
// Created by Ryan Nicholl on 10/23/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_HEADER

#include <cstddef>
namespace rylang
{
    struct type_placement_info
    {
        std::size_t size;
        std::size_t alignment;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_HEADER
