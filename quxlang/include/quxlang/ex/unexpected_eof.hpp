//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_EX_UNEXPECTED_EOF_HEADER_GUARD
#define QUXLANG_EX_UNEXPECTED_EOF_HEADER_GUARD

namespace quxlang
{
    template < typename It >
    struct unexpected_eof
    {
        It pos;
    };
} // namespace quxlang

#endif // QUXLANG_UNEXPECTED_EOF_HEADER_GUARD
