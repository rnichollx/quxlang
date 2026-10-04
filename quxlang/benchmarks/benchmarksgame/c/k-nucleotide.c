#define _POSIX_C_SOURCE 200809L
#include "vendor/khash.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

KHASH_MAP_INIT_INT64(counts, uint64_t)

/** Accumulates every k-mer in one reading frame, growing the library table on demand. */
void accumulate_frame(const unsigned char* sequence, size_t length, size_t width, size_t frame, khash_t(counts) * counts)
{
    if (length < width)
    {
        return;
    }
    for (size_t begin = frame; begin <= length - width; begin += width)
    {
        uint64_t key = 0;
        for (size_t offset = 0; offset < width; ++offset)
        {
            key = (key << 2) | sequence[begin + offset];
        }
        int inserted;
        khiter_t index = kh_put(counts, counts, key, &inserted);
        if (inserted < 0)
        {
            abort();
        }
        if (inserted)
        {
            kh_value(counts, index) = 0;
        }
        ++kh_value(counts, index);
    }
}

/** Counts all reading frames in a fresh library hash table. */
khash_t(counts) * count_nucleotides(const unsigned char* sequence, size_t length, size_t width)
{
    khash_t(counts)* counts = kh_init(counts);
    if (!counts)
    {
        abort();
    }
    for (size_t frame = 0; frame < width; ++frame)
    {
        accumulate_frame(sequence, length, width, frame, counts);
    }
    return counts;
}

/** Writes short k-mer frequencies sorted by descending count and then ascending key. */
void write_frequencies(const unsigned char* sequence, size_t length, size_t width)
{
    khash_t(counts)* counts = count_nucleotides(sequence, length, width);
    uint64_t keys[16], values[16];
    size_t size = 0;
    for (uint64_t key = 0; key < (UINT64_C(1) << (2 * width)); ++key)
    {
        khiter_t found = kh_get(counts, counts, key);
        if (found != kh_end(counts))
        {
            keys[size] = key;
            values[size++] = kh_value(counts, found);
        }
    }
    for (size_t index = 1; index < size; ++index)
    {
        for (size_t position = index; position && values[position] > values[position - 1]; --position)
        {
            uint64_t key = keys[position], value = values[position];
            keys[position] = keys[position - 1];
            values[position] = values[position - 1];
            keys[position - 1] = key;
            values[position - 1] = value;
        }
    }
    for (size_t index = 0; index < size; ++index)
    {
        for (size_t digit = 0; digit < width; ++digit)
        {
            putchar("ACGT"[(keys[index] >> (2 * (width - digit - 1))) & 3]);
        }
        printf(" %.3f\n", 100.0 * (double)values[index] / (double)(length - width + 1));
    }
    putchar('\n');
    kh_destroy(counts, counts);
}

/** Writes one requested sequence's count after counting all sequences of that width. */
void write_count(const unsigned char* sequence, size_t length, const char* query)
{
    size_t width = strlen(query);
    khash_t(counts)* counts = count_nucleotides(sequence, length, width);
    uint64_t key = 0;
    for (size_t index = 0; index < width; ++index)
    {
        unsigned int code = query[index] == 'C' ? 1 : query[index] == 'G' ? 2 : query[index] == 'T' ? 3 : 0;
        key = (key << 2) | code;
    }
    khiter_t found = kh_get(counts, counts, key);
    printf("%" PRIu64 "\t%s\n", found == kh_end(counts) ? UINT64_C(0) : kh_value(counts, found), query);
    kh_destroy(counts, counts);
}

/** Reads FASTA sequence THREE and runs every required counting width. */
int main(void)
{
    char* line = NULL;
    size_t line_capacity = 0, length = 0, capacity = 0;
    unsigned char* sequence = NULL;
    int selected = 0;
    ssize_t read_size;
    while ((read_size = getline(&line, &line_capacity, stdin)) >= 0)
    {
        if (read_size && line[0] == '>')
        {
            if (selected)
            {
                break;
            }
            selected = strncmp(line, ">THREE", 6) == 0;
        }
        else if (selected)
        {
            for (size_t index = 0; index < (size_t)read_size; ++index)
            {
                unsigned char value = (unsigned char)line[index];
                if (value == '\r' || value == '\n')
                {
                    continue;
                }
                value &= 223;
                unsigned char code;
                if (value == 'A')
                {
                    code = 0;
                }
                else if (value == 'C')
                {
                    code = 1;
                }
                else if (value == 'G')
                {
                    code = 2;
                }
                else if (value == 'T')
                {
                    code = 3;
                }
                else
                {
                    abort();
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
                sequence[length++] = code;
            }
        }
    }
    if (ferror(stdin))
    {
        abort();
    }
    free(line);
    write_frequencies(sequence, length, 1);
    write_frequencies(sequence, length, 2);
    const char* queries[] = {"GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"};
    for (size_t index = 0; index < 5; ++index)
    {
        write_count(sequence, length, queries[index]);
    }
    free(sequence);
    return ferror(stdout) ? 1 : 0;
}
