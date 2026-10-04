#ifndef QUXLANG_BENCHMARKSGAME_CPP_BINARY_TREES_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_BINARY_TREES_HPP
#include <cstdint>
#include <memory>

/** Owns a perfect binary tree with one ordinary allocation for every node. */
struct tree_node
{
    std::unique_ptr< tree_node > left;
    std::unique_ptr< tree_node > right;

    /** Allocates the descendants at the requested depth. */
    explicit tree_node(std::uint32_t depth)
    {
        if (depth != 0)
        {
            left = std::make_unique< tree_node >(depth - 1);
            right = std::make_unique< tree_node >(depth - 1);
        }
    }

    /** Counts every allocated node. */
    std::uint64_t check() const
    {
        return left ? 1 + left->check() + right->check() : 1;
    }
};

#endif
