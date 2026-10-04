#ifndef QUXLANG_BENCHMARKSGAME_C_REGEX_REDUX_H
#define QUXLANG_BENCHMARKSGAME_C_REGEX_REDUX_H
#include <stddef.h>

/** Owns a byte sequence and its allocated capacity. */
typedef struct byte_buffer
{
    char* data;
    size_t size;
    size_t capacity;
} byte_buffer;

/** Appends bytes, growing the allocation geometrically. */
void buffer_append(byte_buffer* buffer, char const* data, size_t count);
/** Counts nonoverlapping POSIX extended regular-expression matches. */
size_t regex_count(byte_buffer const* input, char const* pattern);
/** Replaces all nonoverlapping matches with literal bytes. */
byte_buffer regex_replace(byte_buffer const* input, char const* pattern, char const* replacement);
#endif
