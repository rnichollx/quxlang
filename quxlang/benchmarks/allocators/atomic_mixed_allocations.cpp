#include "barriers.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <mimalloc-new-delete.h>

/** Owns pointer slots for one size of uninitialized payload. */
template < std::size_t Bytes >
struct atomic_mixed_slots
{
    /** Matches the Quxlang payload size and alignment. */
    struct block { std::uint64_t words[Bytes / 8]; };
    std::unique_ptr< std::atomic< block* >[] > pointers;
    std::size_t count = 0;

    /** Creates null atomic slots before any worker starts. */
    void initialize(std::size_t slot_count)
    {
        count = slot_count;
        pointers = std::make_unique< std::atomic< block* >[] >(count);
        for (std::size_t index = 0; index < count; ++index)
        { pointers[index].store(nullptr, std::memory_order_relaxed); }
    }

    /** Publishes a replacement and frees the pointer removed by an acquire-release swap. */
    void replace(std::size_t index)
    {
        block* replacement = new block;
        ALLOCATION_WRITE_BYTE(replacement);
        block* previous = pointers[index].load(std::memory_order_relaxed);
        while (!pointers[index].compare_exchange_strong(previous, replacement,
                                                       std::memory_order_acq_rel,
                                                       std::memory_order_relaxed)) { }
        if (previous != nullptr) { delete previous; }
    }

    /** Removes and releases every remaining allocation after all workers join. */
    std::size_t release()
    {
        std::size_t live = 0;
        for (std::size_t index = 0; index < count; ++index)
        {
            block* previous = pointers[index].load(std::memory_order_relaxed);
            while (!pointers[index].compare_exchange_strong(previous, nullptr,
                                                           std::memory_order_acq_rel,
                                                           std::memory_order_relaxed)) { }
            if (previous != nullptr) { delete previous; ++live; }
        }
        return live;
    }
};

/** Owns the eight atomic slot arrays shared by every worker. */
struct atomic_mixed_shared_state
{
    std::size_t slots = 0;
    atomic_mixed_slots< 8 > size_8;
    atomic_mixed_slots< 16 > size_16;
    atomic_mixed_slots< 24 > size_24;
    atomic_mixed_slots< 32 > size_32;
    atomic_mixed_slots< 48 > size_48;
    atomic_mixed_slots< 64 > size_64;
    atomic_mixed_slots< 128 > size_128;
    atomic_mixed_slots< 256 > size_256;

};

/** Borrows shared arrays and owns one worker's pre-generated input. */
struct atomic_mixed_worker_state
{
    std::vector< std::uint32_t > sequence;
    atomic_mixed_shared_state* shared = nullptr;

    /** Applies the worker's operation sequence and writes one byte per allocation. */
    void run()
    {
        for (std::uint32_t word : sequence)
        {
            std::size_t size_index = word % 8;
            std::size_t index = (word / 8) % shared->slots;
            if (size_index == 0) { shared->size_8.replace(index); }
            else if (size_index == 1) { shared->size_16.replace(index); }
            else if (size_index == 2) { shared->size_24.replace(index); }
            else if (size_index == 3) { shared->size_32.replace(index); }
            else if (size_index == 4) { shared->size_48.replace(index); }
            else if (size_index == 5) { shared->size_64.replace(index); }
            else if (size_index == 6) { shared->size_128.replace(index); }
            else if (size_index == 7) { shared->size_256.replace(index); }
        }
    }
};

/** Generates worker inputs or times thread creation through the final join. */
int main(int argc, char** argv)
{
    if (argc != 4) { return 2; }
    std::size_t n = std::strtoull(argv[1], nullptr, 10);
    std::size_t slots = std::strtoull(argv[2], nullptr, 10);
    std::size_t threads = std::strtoull(argv[3], nullptr, 10);
    if (n == 0 || threads == 0) { return 2; }
    if (slots == 0)
    {
        for (std::size_t worker = 0; worker < threads; ++worker)
        {
            std::mt19937 generator(static_cast< std::uint32_t >(20261009 + worker));
            std::size_t count = n;
            for (std::size_t index = 0; index < count; ++index)
            {
                std::uint32_t word = generator();
                unsigned char bytes[4];
                for (unsigned int byte = 0; byte < 4; ++byte)
                { bytes[byte] = static_cast< unsigned char >(word >> (8 * byte)); }
                if (std::fwrite(bytes, 1, 4, stdout) != 4) { return 3; }
            }
        }
        return 0;
    }
    if (slots > 536870912) { return 2; }
    atomic_mixed_shared_state shared;
    shared.slots = slots;
    shared.size_8.initialize(slots);
    shared.size_16.initialize(slots);
    shared.size_24.initialize(slots);
    shared.size_32.initialize(slots);
    shared.size_48.initialize(slots);
    shared.size_64.initialize(slots);
    shared.size_128.initialize(slots);
    shared.size_256.initialize(slots);
    std::vector< atomic_mixed_worker_state > states(threads);
    std::uint64_t checksum = 0;
    for (std::size_t worker = 0; worker < threads; ++worker)
    {
        atomic_mixed_worker_state& state = states[worker];
        state.shared = &shared;
        state.sequence.resize(n);
        for (std::uint32_t& word : state.sequence)
        {
            unsigned char bytes[4];
            if (std::fread(bytes, 1, 4, stdin) != 4) { return 3; }
            word = 0;
            for (unsigned int byte = 0; byte < 4; ++byte)
            { word |= static_cast< std::uint32_t >(bytes[byte]) << (8 * byte); }
            checksum ^= word;
        }
    }
    std::vector< std::thread > workers;
    workers.reserve(threads);
    std::uint64_t start = ALLOCATION_TICKS();
    for (atomic_mixed_worker_state& state : states)
    {
        workers.emplace_back([&state]() { state.run(); });
    }
    for (std::thread& worker : workers) { worker.join(); }
    std::uint64_t elapsed = ALLOCATION_TICKS() - start;
    std::size_t live = 0;
    std::size_t live_bytes = 0;
    {
        std::size_t count;
        count = shared.size_8.release();
        live += count; live_bytes += count * 8;
        count = shared.size_16.release();
        live += count; live_bytes += count * 16;
        count = shared.size_24.release();
        live += count; live_bytes += count * 24;
        count = shared.size_32.release();
        live += count; live_bytes += count * 32;
        count = shared.size_48.release();
        live += count; live_bytes += count * 48;
        count = shared.size_64.release();
        live += count; live_bytes += count * 64;
        count = shared.size_128.release();
        live += count; live_bytes += count * 128;
        count = shared.size_256.release();
        live += count; live_bytes += count * 256;
    }
    std::printf("%llu %llu %llu %llu %llu\n", static_cast< unsigned long long >(elapsed),
                static_cast< unsigned long long >(ALLOCATION_FREQUENCY()),
                static_cast< unsigned long long >(checksum), static_cast< unsigned long long >(live),
                static_cast< unsigned long long >(live_bytes));
}
