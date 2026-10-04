/* Portable serial implementation of the Benchmarks Game fannkuch-redux algorithm. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

/** Enumerates permutations, prints the alternating checksum and maximum flip count. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    size_t n = strtoull(argv[1], NULL, 10);
    if (n < 1 || n > 12)
    {
        return 2;
    }
    size_t* permutation = calloc((size_t)n, sizeof(size_t));
    size_t* scratch = calloc((size_t)n, sizeof(size_t));
    size_t* count = calloc((size_t)n, sizeof(size_t));
    if (!permutation || !scratch || !count)
    {
        abort();
    }
    for (size_t i = 0; i < n; ++i)
    {
        permutation[i] = i;
    }
    size_t rank = n;
    uint64_t maximum = 0;
    int64_t checksum = 0;
    int32_t sign = 1;
    for (;;)
    {
        while (rank > 1)
        {
            count[rank - 1] = rank;
            --rank;
        }
        for (size_t i = 0; i < n; ++i)
        {
            scratch[i] = permutation[i];
        }
        uint64_t flips = 0;
        while (scratch[0] != 0)
        {
            size_t last = scratch[0];
            for (size_t left = 0; left < (last + 1) / 2; ++left)
            {
                int saved = scratch[left];
                scratch[left] = scratch[last - left];
                scratch[last - left] = saved;
            }
            ++flips;
        }
        if (flips > maximum)
        {
            maximum = flips;
        }
        if (sign > 0)
        {
            checksum += (int64_t)flips;
        }
        else
        {
            checksum -= (int64_t)flips;
        }
        sign = -sign;
        for (;;)
        {
            if (rank == n)
            {
                printf("%" PRId64 "\nPfannkuchen(%zu) = %" PRIu64 "\n", checksum, n, maximum);
                free(count);
                free(scratch);
                free(permutation);
                return 0;
            }
            size_t first = permutation[0];
            for (size_t i = 0; i < rank; ++i)
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
