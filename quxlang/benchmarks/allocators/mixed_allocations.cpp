#include "barriers.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
#include <mimalloc-new-delete.h>

/** Owns pointer slots for one size of uninitialized payload. */
template < std::size_t Bytes >
struct mixed_slots
{
    /** Matches the Quxlang payload size and alignment. */
    struct block { std::uint64_t words[Bytes / 8]; };
    std::vector< block* > pointers;

    /** Allocates an empty slot or releases an occupied slot. */
    void toggle(std::size_t index)
    {
        block*& pointer = pointers[index];
        if (pointer == nullptr)
        {
            pointer = new block;
            DO_NOT_OPTIMIZE(pointer);
        }
        else
        {
            delete pointer;
            pointer = nullptr;
        }
    }

    /** Releases remaining allocations after timing and returns their count. */
    std::size_t release()
    {
        std::size_t live = 0;
        for (block*& pointer : pointers)
        {
            if (pointer != nullptr)
            {
                delete pointer;
                pointer = nullptr;
                ++live;
            }
        }
        return live;
    }
};

/** Generates the shared MT19937 input or measures its allocation workload. */
int main(int argc, char** argv)
{
    if (argc != 3) { return 2; }
    std::size_t n = std::strtoull(argv[1], nullptr, 10);
    std::size_t slots = std::strtoull(argv[2], nullptr, 10);
    if (n == 0) { return 2; }
    if (slots == 0)
    {
        std::mt19937 generator(20261009);
        for (std::size_t index = 0; index < n; ++index)
        {
            std::uint32_t word = generator();
            unsigned char bytes[4];
            for (unsigned int byte = 0; byte < 4; ++byte)
            { bytes[byte] = static_cast< unsigned char >(word >> (8 * byte)); }
            if (std::fwrite(bytes, 1, 4, stdout) != 4) { return 3; }
        }
        return 0;
    }
    if (slots > 536870912) { return 2; }
    std::vector< std::uint32_t > sequence(n);
    std::uint64_t checksum = 0;
    for (std::uint32_t& word : sequence)
    {
        unsigned char bytes[4];
        if (std::fread(bytes, 1, 4, stdin) != 4) { return 3; }
        word = 0;
        for (unsigned int byte = 0; byte < 4; ++byte)
        { word |= static_cast< std::uint32_t >(bytes[byte]) << (8 * byte); }
        checksum += word;
    }
    mixed_slots< 8 > size_8;
    size_8.pointers.resize(slots, nullptr);
    mixed_slots< 16 > size_16;
    size_16.pointers.resize(slots, nullptr);
    mixed_slots< 24 > size_24;
    size_24.pointers.resize(slots, nullptr);
    mixed_slots< 32 > size_32;
    size_32.pointers.resize(slots, nullptr);
    mixed_slots< 48 > size_48;
    size_48.pointers.resize(slots, nullptr);
    mixed_slots< 64 > size_64;
    size_64.pointers.resize(slots, nullptr);
    mixed_slots< 128 > size_128;
    size_128.pointers.resize(slots, nullptr);
    mixed_slots< 256 > size_256;
    size_256.pointers.resize(slots, nullptr);
    std::uint64_t start = ALLOCATION_TICKS();
    for (std::uint32_t word : sequence)
    {
        std::size_t size_index = word % 8;
        std::size_t index = (word / 8) % slots;
        if (size_index == 0) { size_8.toggle(index); }
        else if (size_index == 1) { size_16.toggle(index); }
        else if (size_index == 2) { size_24.toggle(index); }
        else if (size_index == 3) { size_32.toggle(index); }
        else if (size_index == 4) { size_48.toggle(index); }
        else if (size_index == 5) { size_64.toggle(index); }
        else if (size_index == 6) { size_128.toggle(index); }
        else if (size_index == 7) { size_256.toggle(index); }
    }
    std::uint64_t elapsed = ALLOCATION_TICKS() - start;
    std::size_t live = 0;
    std::size_t live_bytes = 0;
    std::size_t count;
    count = size_8.release();
    live += count; live_bytes += count * 8;
    count = size_16.release();
    live += count; live_bytes += count * 16;
    count = size_24.release();
    live += count; live_bytes += count * 24;
    count = size_32.release();
    live += count; live_bytes += count * 32;
    count = size_48.release();
    live += count; live_bytes += count * 48;
    count = size_64.release();
    live += count; live_bytes += count * 64;
    count = size_128.release();
    live += count; live_bytes += count * 128;
    count = size_256.release();
    live += count; live_bytes += count * 256;
    std::printf("%llu %llu %llu %llu %llu\n", static_cast< unsigned long long >(elapsed),
                static_cast< unsigned long long >(ALLOCATION_FREQUENCY()),
                static_cast< unsigned long long >(checksum), static_cast< unsigned long long >(live),
                static_cast< unsigned long long >(live_bytes));
}
