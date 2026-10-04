#include "reverse-complement.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

void write_complement(std::string_view header, std::span< std::uint8_t const > sequence, std::array< std::uint8_t, 256 > const& complements, output_buffer& output)
{
    output.write_text(header);
    output.write_byte('\n');
    for (std::size_t index = 0; index < sequence.size(); ++index)
    {
        output.write_byte(complements[sequence[sequence.size() - 1 - index]]);
        if (index % 60 == 59 || index + 1 == sequence.size())
        {
            output.write_byte('\n');
        }
    }
}

/** Reads records line by line and grows sequence storage through std::vector. */
int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    output_buffer output;
    std::array< std::uint8_t, 256 > complements{};
    std::string_view letters = "ACGTUMRWSYKVHDBN";
    std::string_view paired = "TGCAAKYWSRMBDHVN";
    for (std::size_t i = 0; i < letters.size(); ++i)
    {
        complements[static_cast< std::uint8_t >(letters[i])] = paired[i];
        complements[static_cast< std::uint8_t >(letters[i]) + 32] = paired[i];
    }
    std::string line;
    std::string header;
    std::vector< std::uint8_t > sequence;
    while (std::getline(std::cin, line))
    {
        if (!line.empty() && line.front() == '>')
        {
            if (!header.empty())
            {
                write_complement(header, sequence, complements, output);
            }
            header = line;
            sequence.clear();
        }
        else
        {
            for (std::uint8_t value : line)
            {
                if (value != '\r')
                {
                    sequence.push_back(value);
                }
            }
        }
    }
    if (std::cin.bad())
    {
        std::abort();
    }
    if (!header.empty())
    {
        write_complement(header, sequence, complements, output);
    }
    output.flush();
}
