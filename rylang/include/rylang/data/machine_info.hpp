//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RYLANG_MACHINE_INFO_HEADER_GUARD
#define RYLANG_MACHINE_INFO_HEADER_GUARD

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

#endif // RYLANG_MACHINE_INFO_HEADER_GUARD
