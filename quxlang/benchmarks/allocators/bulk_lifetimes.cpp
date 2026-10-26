#include "barriers.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

#if defined(USE_MIMALLOC)
#include <mimalloc-new-delete.h>
#endif

/** The workload retains a complete balanced tree before recursively releasing its nodes. */
template < std::size_t Bytes >
struct bulk_lifetime_workload
{
    struct node
    {
        node* left;
        node* right;
        std::uint64_t payload[Bytes / 8 - 2];
    };

    static_assert(sizeof(node) == Bytes);

    /** The builder creates exactly count nodes with consecutive identities in preorder. */
    static node* build(std::size_t count, std::uint64_t first)
    {
        if (count == 0) { return nullptr; }
        node* result = new node{};
        for (std::size_t index = 0; index < Bytes / 8 - 2; index += 512)
        { ALLOCATION_WRITE_WORD(&result->payload[index], first); }
        ALLOCATION_WRITE_WORD(&result->payload[Bytes / 8 - 3], first);
        std::size_t left_count = count / 2;
        result->left = build(left_count, first + 1);
        result->right = build(count - left_count - 1, first + left_count + 1);
        return result;
    }

    /** The postorder traversal checks initialized payload words and deletes every node once. */
    static std::uint64_t dispose(node* tree, std::uint64_t& freed)
    {
        if (tree == nullptr) { return 0; }
        std::uint64_t checksum = dispose(tree->left, freed);
        checksum += dispose(tree->right, freed);
        for (std::size_t index = 0; index < Bytes / 8 - 2; index += 512)
        { checksum += tree->payload[index]; }
        checksum += tree->payload[Bytes / 8 - 3];
        ++freed;
        delete tree;
        return checksum;
    }

    /** Each cycle measures the two phases separately at a fixed live-node budget. */
    static void run(std::size_t cycles, std::size_t memory)
    {
        std::size_t count = memory / Bytes;
        std::uint64_t allocated_ticks = 0;
        std::uint64_t deallocated_ticks = 0;
        std::uint64_t checksum = 0;
        std::uint64_t freed = 0;
        for (std::size_t cycle = 0; cycle < cycles; ++cycle)
        {
            std::uint64_t start = ALLOCATION_TICKS();
            node* root = build(count, 1);
            DO_NOT_OPTIMIZE(root);
            std::uint64_t allocated = ALLOCATION_TICKS();
            checksum += dispose(root, freed);
            std::uint64_t finished = ALLOCATION_TICKS();
            allocated_ticks += allocated - start;
            deallocated_ticks += finished - allocated;
        }
        std::printf("%llu %llu %llu %llu %llu\n",
                    static_cast< unsigned long long >(allocated_ticks),
                    static_cast< unsigned long long >(deallocated_ticks),
                    static_cast< unsigned long long >(ALLOCATION_FREQUENCY()),
                    static_cast< unsigned long long >(checksum),
                    static_cast< unsigned long long >(freed));
    }
};

/** The parser accepts one positive size_t value without trailing characters. */
static bool parse_count(const char* text, std::size_t& result)
{
    if (*text < '0' || *text > '9') { return false; }
    char* end = nullptr;
    errno = 0;
    auto value = std::strtoull(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || value == 0 || value > std::numeric_limits< std::size_t >::max())
    { return false; }
    result = static_cast< std::size_t >(value);
    return true;
}

/** The entry point selects the matched node size and live-node object-byte budget. */
int main(int argc, char** argv)
{
    if (argc != 4 || sizeof(std::size_t) != 8) { return 2; }
    std::size_t cycles;
    std::size_t size;
    std::size_t memory;
    if (!parse_count(argv[1], cycles) || !parse_count(argv[2], size)
        || !parse_count(argv[3], memory) || memory < size) { return 2; }
    if (size == 32) { bulk_lifetime_workload< 32 >::run(cycles, memory); return 0; }
    if (size == 64) { bulk_lifetime_workload< 64 >::run(cycles, memory); return 0; }
    if (size == 256) { bulk_lifetime_workload< 256 >::run(cycles, memory); return 0; }
    if (size == 1024) { bulk_lifetime_workload< 1024 >::run(cycles, memory); return 0; }
    if (size == 4096) { bulk_lifetime_workload< 4096 >::run(cycles, memory); return 0; }
    if (size == 8192) { bulk_lifetime_workload< 8192 >::run(cycles, memory); return 0; }
    return 2;
}
