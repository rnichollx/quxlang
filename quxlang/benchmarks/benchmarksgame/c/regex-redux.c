#define _POSIX_C_SOURCE 200809L
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif
#include "regex-redux.h"
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buffer_append(byte_buffer* buffer, char const* data, size_t count)
{
    if (count > SIZE_MAX - buffer->size - 1)
    {
        abort();
    }
    size_t required = buffer->size + count + 1;
    if (required > buffer->capacity)
    {
        size_t capacity = buffer->capacity ? buffer->capacity : 16384;
        while (capacity < required)
        {
            if (capacity > SIZE_MAX / 2)
            {
                capacity = required;
                break;
            }
            capacity *= 2;
        }
        char* allocation = realloc(buffer->data, capacity);
        if (!allocation)
        {
            abort();
        }
        buffer->data = allocation;
        buffer->capacity = capacity;
    }
    if (count)
    {
        memcpy(buffer->data + buffer->size, data, count);
    }
    buffer->size += count;
    buffer->data[buffer->size] = 0;
}

size_t regex_count(byte_buffer const* input, char const* pattern)
{
    regex_t expression;
    if (regcomp(&expression, pattern, REG_EXTENDED | REG_NEWLINE))
    {
        abort();
    }
    size_t count = 0;
    size_t offset = 0;
    while (offset <= input->size)
    {
        regmatch_t match = {(regoff_t)offset, (regoff_t)input->size};
        int status = regexec(&expression, input->data, 1, &match, REG_STARTEND);
        if (status == REG_NOMATCH)
        {
            break;
        }
        if (status)
        {
            abort();
        }
        ++count;
        offset = (size_t)match.rm_eo;
        if (match.rm_so == match.rm_eo)
        {
            if (offset == input->size)
            {
                break;
            }
            ++offset;
        }
    }
    regfree(&expression);
    return count;
}

byte_buffer regex_replace(byte_buffer const* input, char const* pattern, char const* replacement)
{
    regex_t expression;
    if (regcomp(&expression, pattern, REG_EXTENDED | REG_NEWLINE))
    {
        abort();
    }
    byte_buffer output = {0};
    size_t offset = 0;
    size_t copied = 0;
    size_t replacement_size = strlen(replacement);
    while (offset <= input->size)
    {
        regmatch_t match = {(regoff_t)offset, (regoff_t)input->size};
        int status = regexec(&expression, input->data, 1, &match, REG_STARTEND);
        if (status == REG_NOMATCH)
        {
            break;
        }
        if (status)
        {
            abort();
        }
        buffer_append(&output, input->data + copied, (size_t)match.rm_so - copied);
        buffer_append(&output, replacement, replacement_size);
        copied = (size_t)match.rm_eo;
        offset = copied;
        if (match.rm_so == match.rm_eo)
        {
            if (offset == input->size)
            {
                break;
            }
            ++offset;
        }
    }
    buffer_append(&output, input->data + copied, input->size - copied);
    regfree(&expression);
    return output;
}

/** Counts DNA patterns and performs the prescribed sequential regex replacements. */
int main(void)
{
    byte_buffer input = {0};
    char block[16384];
    for (size_t count; (count = fread(block, 1, sizeof block, stdin)) != 0;)
    {
        buffer_append(&input, block, count);
    }
    if (ferror(stdin))
    {
        return EXIT_FAILURE;
    }
    buffer_append(&input, "", 0);
    size_t original_length = input.size;
    byte_buffer sequence = regex_replace(&input, ">.*\n|\n", "");
    free(input.data);
    size_t sequence_length = sequence.size;
    char const* patterns[] = {"agggtaaa|tttaccct", "[cgt]gggtaaa|tttaccc[acg]", "a[act]ggtaaa|tttacc[agt]t", "ag[act]gtaaa|tttac[agt]ct", "agg[act]taaa|ttta[agt]cct", "aggg[acg]aaa|ttt[cgt]ccct", "agggt[cgt]aa|tt[acg]accct", "agggta[cgt]a|t[acg]taccct", "agggtaa[cgt]|[acg]ttaccct"};
    for (size_t i = 0; i < sizeof patterns / sizeof patterns[0]; ++i)
    {
        printf("%s %zu\n", patterns[i], regex_count(&sequence, patterns[i]));
    }
    char const* substitutions[] = {"tHa[Nt]", "aND|caN|Ha[DS]|WaS", "a[NSt]|BY", "<[^>]*>", "\\|[^|][^|]*\\|"};
    char const* replacements[] = {"<4>", "<3>", "<2>", "|", "-"};
    for (size_t i = 0; i < sizeof substitutions / sizeof substitutions[0]; ++i)
    {
        byte_buffer replaced = regex_replace(&sequence, substitutions[i], replacements[i]);
        free(sequence.data);
        sequence = replaced;
    }
    printf("\n%zu\n%zu\n%zu\n", original_length, sequence_length, sequence.size);
    free(sequence.data);
    return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}
