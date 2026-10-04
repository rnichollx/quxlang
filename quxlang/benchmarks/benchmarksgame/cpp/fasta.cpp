#include "fasta.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>

/** Generates a weighted random sequence with the prescribed LCG and cumulative probabilities. */
void random_sequence(std::string_view letters, std::span< double const > probabilities, std::uint32_t& seed, std::size_t length, output_buffer& output)
{
    for (std::size_t index = 0; index < length; ++index)
    {
        seed = (seed * 3877 + 29573) % 139968;
        double random = static_cast< double >(seed) / 139968.0;
        for (std::size_t choice = 0; choice < probabilities.size(); ++choice)
        {
            if (random < probabilities[choice])
            {
                output.write_byte(static_cast< std::uint8_t >(letters[choice]));
                break;
            }
        }
        if (index % 60 == 59 || index + 1 == length)
        {
            output.write_byte('\n');
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
    std::size_t n = std::strtoul(argv[1], nullptr, 10);
    std::string_view alu = "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";
    output_buffer output;
    output.write_text(">ONE Homo sapiens alu\n");
    std::size_t alu_length = alu.size();
    for (std::size_t index = 0; index < n * 2; ++index)
    {
        output.write_byte(static_cast< std::uint8_t >(alu[index % alu_length]));
        if (index % 60 == 59 || index + 1 == n * 2)
        {
            output.write_byte('\n');
        }
    }
    std::array< double, 15 > iub = {0.27, 0.12, 0.12, 0.27, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02};
    std::array< double, 4 > human = {0.3029549426680, 0.1979883004921, 0.1975473066391, 0.3015094502008};
    for (std::size_t i = 1; i < 15; ++i)
    {
        iub[i] += iub[i - 1];
    }
    for (std::size_t i = 1; i < 4; ++i)
    {
        human[i] += human[i - 1];
    }
    std::uint32_t seed = 42;
    output.write_text(">TWO IUB ambiguity codes\n");
    random_sequence("acgtBDHKMNRSVWY", iub, seed, n * 3, output);
    output.write_text(">THREE Homo sapiens frequency\n");
    random_sequence("acgt", human, seed, n * 5, output);
    output.flush();
    return 0;
}
