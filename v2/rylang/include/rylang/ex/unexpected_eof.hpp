//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_UNEXPECTED_EOF_HEADER
#define RPNX_RYANSCRIPT1031_UNEXPECTED_EOF_HEADER

namespace rylang
{
    template < typename It >
    struct unexpected_eof
    {
        It pos;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_UNEXPECTED_EOF_HEADER
