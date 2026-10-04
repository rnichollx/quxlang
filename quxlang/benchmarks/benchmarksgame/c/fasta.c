#include "output.h"
#include <stdint.h>
#include <string.h>

/** Generates a weighted random sequence with the prescribed LCG and cumulative probabilities. */
void random_sequence(const char* letters, const double* probabilities, size_t count, uint32_t* seed, size_t length, struct output_buffer* output)
{
    for (size_t index = 0; index < length; ++index)
    {
        *seed = (*seed * 3877 + 29573) % 139968;
        double random = (double)*seed / 139968.0;
        for (size_t choice = 0; choice < count; ++choice)
        {
            if (random < probabilities[choice])
            {
                write_byte(output, (unsigned char)letters[choice]);
                break;
            }
        }
        if (index % 60 == 59 || index + 1 == length)
        {
            write_byte(output, '\n');
        }
    }
}

/** Emits the ALU, IUB, and human-frequency sequences in sixty-column FASTA format. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    size_t n = strtoul(argv[1], NULL, 10);
    const char* alu = "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";
    struct output_buffer output = {{0}, 0};
    write_text(&output, ">ONE Homo sapiens alu\n");
    size_t alu_length = strlen(alu);
    for (size_t index = 0; index < n * 2; ++index)
    {
        write_byte(&output, (unsigned char)alu[index % alu_length]);
        if (index % 60 == 59 || index + 1 == n * 2)
        {
            write_byte(&output, '\n');
        }
    }
    double iub[15] = {0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02};
    double human[4] = {0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008};
    for (size_t i = 1; i < 15; ++i)
    {
        iub[i] += iub[i - 1];
    }
    for (size_t i = 1; i < 4; ++i)
    {
        human[i] += human[i - 1];
    }
    uint32_t seed = 42;
    write_text(&output, ">TWO IUB ambiguity codes\n");
    random_sequence("acgtBDHKMNRSVWY", iub, 15, &seed, n * 3, &output);
    write_text(&output, ">THREE Homo sapiens frequency\n");
    random_sequence("acgt", human, 4, &seed, n * 5, &output);
    flush_output(&output);
    return 0;
}
