#ifndef BENCHMARKSGAME_OUTPUT_H
#define BENCHMARKSGAME_OUTPUT_H
#include <stdio.h>
#include <stdlib.h>

/** A bounded byte buffer for standard output. */
struct output_buffer
{
    unsigned char bytes[16384];
    size_t used;
};

/** Writes pending bytes and reports output failures. */
void flush_output(struct output_buffer* output)
{
    if (fwrite(output->bytes, 1, output->used, stdout) != output->used)
    {
        abort();
    }
    output->used = 0;
}

/** Appends one byte to the output buffer. */
void write_byte(struct output_buffer* output, unsigned char value)
{
    output->bytes[output->used++] = value;
    if (output->used == sizeof(output->bytes))
    {
        flush_output(output);
    }
}

/** Appends a terminated string to the output buffer. */
void write_text(struct output_buffer* output, const char* text)
{
    while (*text)
    {
        write_byte(output, (unsigned char)*text++);
    }
}
#endif
