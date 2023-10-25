//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_MACHINE_INFO_HEADER
#define RPNX_RYANSCRIPT1031_MACHINE_INFO_HEADER

#include <cstddef>
#include <optional>

namespace rylang
{
    struct machine_info
    {
        std::size_t pointer_size = 8;
        std::size_t pointer_align = 8;
        std::optional<std::size_t> max_int_align;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MACHINE_INFO_HEADER
