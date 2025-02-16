// Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_INTTYPES_H
#define QUXLANG_INTTYPES_H

#include <cinttypes>

#if defined(_WIN64)
typedef signed __int64 ssize_t;
#elif defined(_WIN32)
typedef signed int ssize_t;
#endif /* _WIN64 */

#endif // QUXLANG_INTTYPES_H
