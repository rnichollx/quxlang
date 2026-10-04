#include "k-nucleotide.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

void accumulate_frame(std::span< std::uint8_t const > sequence, std::size_t width, std::size_t frame, std::unordered_map< std::uint64_t, std::uint64_t >& counts)
{
    if (sequence.size() < width)
    {
        return;
    }
    for (std::size_t begin = frame; begin <= sequence.size() - width; begin += width)
    {
        std::uint64_t key = 0;
        for (std::size_t offset = 0; offset < width; ++offset)
        {
            key = (key << 2) | sequence[begin + offset];
        }
        ++counts[key];
    }
}

std::unordered_map< std::uint64_t, std::uint64_t > count_nucleotides(std::span< std::uint8_t const > sequence, std::size_t width)
{
    std::unordered_map< std::uint64_t, std::uint64_t > counts;
    for (std::size_t frame = 0; frame < width; ++frame)
    {
        accumulate_frame(sequence, width, frame, counts);
    }
    return counts;
}

void write_frequencies(std::span< std::uint8_t const > sequence, std::size_t width)
{
    std::unordered_map< std::uint64_t, std::uint64_t > counts = count_nucleotides(sequence, width);
    std::vector< std::pair< std::uint64_t, std::uint64_t > > frequencies;
    for (std::uint64_t key = 0; key < (UINT64_C(1) << (2 * width)); ++key)
    {
        std::unordered_map< std::uint64_t, std::uint64_t >::iterator found = counts.find(key);
        if (found != counts.end())
        {
            frequencies.emplace_back(key, found->second);
        }
    }
    std::stable_sort(frequencies.begin(), frequencies.end(),
                     [](std::pair< std::uint64_t, std::uint64_t > const& left, std::pair< std::uint64_t, std::uint64_t > const& right)
                     {
                         return left.second > right.second;
                     });
    for (std::pair< std::uint64_t, std::uint64_t > const& frequency : frequencies)
    {
        for (std::size_t digit = 0; digit < width; ++digit)
        {
            std::putchar("ACGT"[(frequency.first >> (2 * (width - digit - 1))) & 3]);
        }
        std::printf(" %.3f\n", 100.0 * static_cast< double >(frequency.second) / static_cast< double >(sequence.size() - width + 1));
    }
    std::putchar('\n');
}

void write_count(std::span< std::uint8_t const > sequence, std::string_view query)
{
    std::unordered_map< std::uint64_t, std::uint64_t > counts = count_nucleotides(sequence, query.size());
    std::uint64_t key = 0;
    for (char character : query)
    {
        std::uint64_t code = character == 'C' ? 1 : character == 'G' ? 2 : character == 'T' ? 3 : 0;
        key = (key << 2) | code;
    }
    std::unordered_map< std::uint64_t, std::uint64_t >::iterator found = counts.find(key);
    std::cout << (found == counts.end() ? 0 : found->second) << '\t' << query << '\n';
}

/** Reads FASTA sequence THREE and runs every required counting width. */
int main()
{
    std::vector< std::uint8_t > sequence;
    std::string line;
    bool selected = false;
    while (std::getline(std::cin, line))
    {
        if (!line.empty() && line.front() == '>')
        {
            if (selected)
            {
                break;
            }
            selected = line.starts_with(">THREE");
        }
        else if (selected)
        {
            for (char character : line)
            {
                if (character == '\r')
                {
                    continue;
                }
                std::uint8_t value = static_cast< std::uint8_t >(character) & 223;
                if (value == 'A')
                {
                    sequence.push_back(0);
                }
                else if (value == 'C')
                {
                    sequence.push_back(1);
                }
                else if (value == 'G')
                {
                    sequence.push_back(2);
                }
                else if (value == 'T')
                {
                    sequence.push_back(3);
                }
                else
                {
                    std::abort();
                }
            }
        }
    }
    if (std::cin.bad())
    {
        std::abort();
    }
    write_frequencies(sequence, 1);
    write_frequencies(sequence, 2);
    for (std::string_view query : {"GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"})
    {
        write_count(sequence, query);
    }
}
