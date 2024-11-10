// Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_INTTYPES_H
#define QUXLANG_INTTYPES_H

#include <inttypes.h>

#if defined(_WIN64)
typedef signed __int64 ssize_t;
#elif not defined(__ssize_t_defined)
typedef signed int ssize_t;
#endif /* _WIN64 */

#endif // QUXLANG_INTTYPES_H
