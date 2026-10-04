#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>

/** Enumerates permutations using the serial fannkuch-redux algorithm. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    std::size_t n = std::strtoull(argv[1], nullptr, 10);
    if (n < 1 || n > 12)
    {
        return 2;
    }
    std::vector< std::size_t > permutation(n);
    std::vector< std::size_t > scratch(n);
    std::vector< std::size_t > count(n);
    std::iota(permutation.begin(), permutation.end(), 0);
    std::size_t rank = n;
    std::uint64_t maximum = 0;
    std::int64_t checksum = 0;
    std::int32_t sign = 1;
    for (;;)
    {
        while (rank > 1)
        {
            count[rank - 1] = rank;
            --rank;
        }
        std::copy(permutation.begin(), permutation.end(), scratch.begin());
        std::uint64_t flips = 0;
        while (scratch[0] != 0)
        {
            std::size_t last = scratch[0];
            for (std::size_t left = 0; left < (last + 1) / 2; ++left)
            {
                std::swap(scratch[left], scratch[last - left]);
            }
            ++flips;
        }
        maximum = std::max(maximum, flips);
        if (sign > 0)
        {
            checksum += static_cast< std::int64_t >(flips);
        }
        else
        {
            checksum -= static_cast< std::int64_t >(flips);
        }
        sign = -sign;
        for (;;)
        {
            if (rank == n)
            {
                std::printf("%" PRId64 "\nPfannkuchen(%zu) = %" PRIu64 "\n", checksum, n, maximum);
                return 0;
            }
            std::size_t first = permutation[0];
            for (std::size_t i = 0; i < rank; ++i)
            {
                permutation[i] = permutation[i + 1];
            }
            permutation[rank] = first;
            if (--count[rank] != 0)
            {
                break;
            }
            ++rank;
        }
    }
}
