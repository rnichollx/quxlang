// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

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
