//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_UNEXPECTED_EOF_HEADER_GUARD
#define RYLANG_UNEXPECTED_EOF_HEADER_GUARD

namespace rylang
{
    template < typename It >
    struct unexpected_eof
    {
        It pos;
    };
} // namespace rylang

#endif // RYLANG_UNEXPECTED_EOF_HEADER_GUARD
