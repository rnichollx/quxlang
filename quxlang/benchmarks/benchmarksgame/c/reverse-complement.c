#define _POSIX_C_SOURCE 200809L
#include "output.h"
#include <string.h>

/** Emits one complete FASTA record in reverse-complement order. */
void write_complement(const char* header, const unsigned char* sequence, size_t length, const unsigned char* complements, struct output_buffer* output)
{
    write_text(output, header);
    write_byte(output, '\n');
    for (size_t index = 0; index < length; ++index)
    {
        write_byte(output, complements[sequence[length - 1 - index]]);
        if (index % 60 == 59 || index + 1 == length)
        {
            write_byte(output, '\n');
        }
    }
}

/** Reads records line by line and grows sequence storage geometrically. */
int main(void)
{
    struct output_buffer output = {{0}, 0};
    unsigned char complements[256] = {0};
    const char* letters = "ACGTUMRWSYKVHDBN";
    const char* paired = "TGCAAKYWSRMBDHVN";
    for (size_t i = 0; letters[i]; ++i)
    {
        complements[(unsigned char)letters[i]] = (unsigned char)paired[i];
        complements[(unsigned char)letters[i] + 32] = (unsigned char)paired[i];
    }
    char* line = NULL;
    size_t line_capacity = 0;
    char* header = NULL;
    unsigned char* sequence = NULL;
    size_t length = 0, capacity = 0;
    ssize_t read_size;
    while ((read_size = getline(&line, &line_capacity, stdin)) >= 0)
    {
        size_t size = (size_t)read_size;
        if (size && line[size - 1] == '\n')
        {
            line[--size] = 0;
        }
        if (size && line[0] == '>')
        {
            if (header)
            {
                write_complement(header, sequence, length, complements, &output);
            }
            free(header);
            header = strdup(line);
            if (!header)
            {
                abort();
            }
            length = 0;
        }
        else
        {
            for (size_t i = 0; i < size; ++i)
            {
                if (line[i] == '\r')
                {
                    continue;
                }
                if (length == capacity)
                {
                    capacity = capacity ? capacity * 2 : 256;
                    unsigned char* grown = realloc(sequence, capacity);
                    if (!grown)
                    {
                        abort();
                    }
                    sequence = grown;
                }
                sequence[length++] = (unsigned char)line[i];
            }
        }
    }
    if (ferror(stdin))
    {
        abort();
    }
    if (header)
    {
        write_complement(header, sequence, length, complements, &output);
    }
    flush_output(&output);
    free(line);
    free(header);
    free(sequence);
    return 0;
}
