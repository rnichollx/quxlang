//
// Created by Ryan Nicholl on 7/19/2024.
//

#ifndef QUXLANG_INTTYPES_H
#define QUXLANG_INTTYPES_H

#if defined(_WIN64)
typedef signed __int64 ssize_t;
#else
typedef signed int ssize_t;
#endif /* _WIN64 */

#endif // QUXLANG_INTTYPES_H
