// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DEBUG_HEADER_GUARD
#define QUXLANG_DEBUG_HEADER_GUARD

#ifndef NDEBUG
#define QUXLANG_DEBUG(x) x
#else
#define QUXLANG_DEBUG(x)
#endif

#endif //DEBUG_HEADER_GUARD
