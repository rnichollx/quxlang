#include "binary-trees.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

/** Runs the serial binary-trees workload with standard owning pointers. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    std::int32_t requested = std::atoi(argv[1]);
    if (requested < 0 || requested > 24)
    {
        return 2;
    }
    std::uint32_t maximum = static_cast< std::uint32_t >(std::max(6, requested));
    {
        std::unique_ptr< tree_node > stretch = std::make_unique< tree_node >(maximum + 1);
        std::printf("stretch tree of depth %u\t check: %llu\n", maximum + 1, static_cast< unsigned long long >(stretch->check()));
    }
    std::unique_ptr< tree_node > long_lived = std::make_unique< tree_node >(maximum);
    for (std::uint32_t depth = 4; depth <= maximum; depth += 2)
    {
        std::uint64_t iterations = std::uint64_t{1} << (maximum - depth + 4);
        std::uint64_t check = 0;
        for (std::uint64_t i = 0; i < iterations; ++i)
        {
            std::unique_ptr< tree_node > tree = std::make_unique< tree_node >(depth);
            check += tree->check();
        }
        std::printf("%llu\t trees of depth %u\t check: %llu\n", static_cast< unsigned long long >(iterations), depth, static_cast< unsigned long long >(check));
    }
    std::printf("long lived tree of depth %u\t check: %llu\n", maximum, static_cast< unsigned long long >(long_lived->check()));
}
