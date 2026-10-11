#include "barriers.h"
#include <array>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <mimalloc-new-delete.h>

/** A bounded FIFO of borrowed pointers, including null termination markers. */
template < typename T, std::size_t Capacity >
class synchronized_pointer_queue
{
    static_assert(Capacity > 0);
    std::array< T*, Capacity > entries{};
    std::size_t head = 0;
    std::size_t count = 0;
    std::mutex mutex;
    std::condition_variable readable;
    std::condition_variable writable;

public:
    /** Publishes a pointer after waiting for an available slot. */
    void push(T* value)
    {
        std::unique_lock< std::mutex > lock(mutex);
        writable.wait(lock, [this] { return count < Capacity; });
        entries[(head + count) % Capacity] = value;
        ++count;
        readable.notify_one();
    }

    /** Removes the oldest pointer after waiting for an entry. */
    T* pop()
    {
        std::unique_lock< std::mutex > lock(mutex);
        readable.wait(lock, [this] { return count != 0; });
        T* value = entries[head];
        entries[head] = nullptr;
        head = (head + 1) % Capacity;
        --count;
        writable.notify_one();
        return value;
    }
};

/** Contains two integers consumed by the checksum recurrence. */
struct producer_consumer_object { std::uint64_t first; std::uint64_t second; };
/** Owns 128 separately allocated objects during queue transfer. */
struct producer_consumer_block { producer_consumer_object* objects[128]; };
static_assert(sizeof(producer_consumer_object) == 16);
static_assert(sizeof(producer_consumer_block) == 128 * sizeof(void*));

/** Generates deterministic SplitMix64 values with wrapping arithmetic. */
struct producer_consumer_random
{
    std::uint64_t state;
    /** Advances SplitMix64 and returns its scrambled value. */
    std::uint64_t next()
    {
        state += UINT64_C(11400714819323198485);
        std::uint64_t value = state;
        value = (value ^ (value >> 30)) * UINT64_C(13787848793156543929);
        value = (value ^ (value >> 27)) * UINT64_C(10723151780598845931);
        return value ^ (value >> 31);
    }
};

/** Holds a stable queue and parameters shared by one thread pair. */
struct producer_consumer_pair
{
    synchronized_pointer_queue< producer_consumer_block, 64 > queue;
    std::size_t identity = 0;
    std::size_t batches = 0;
    std::mutex* io_mutex = nullptr;

    /** Allocates batches and publishes a null pointer after the final batch. */
    void produce()
    {
        producer_consumer_random random{20261011 + identity};
        for (std::size_t index = 0; index < batches; ++index)
        {
            producer_consumer_block* block = new producer_consumer_block;
            for (producer_consumer_object*& object : block->objects)
            {
                object = new producer_consumer_object;
                object->first = random.next();
                object->second = random.next() | 1;
            }
            queue.push(block);
        }
        queue.push(nullptr);
    }

    /** Computes two checksums, releases each batch and prints under the shared I/O mutex. */
    void consume()
    {
        std::uint64_t first = UINT64_C(14695981039346656037);
        std::uint64_t second = UINT64_C(1099511628211);
        while (producer_consumer_block* block = queue.pop())
        {
            for (producer_consumer_object* object : block->objects)
            {
                first = (first ^ object->first) * object->second;
                second = (second ^ object->first) * object->second;
                delete object;
            }
            delete block;
        }
        std::lock_guard< std::mutex > lock(*io_mutex);
        std::printf("%zu %llu %llu\n", identity,
                    static_cast< unsigned long long >(first), static_cast< unsigned long long >(second));
        std::fflush(stdout);
    }
};

/** Times producer and consumer creation through all joins, including remote frees and TLS teardown. */
int main(int argc, char** argv)
{
    if (argc != 3) { return 1; }
    std::size_t batches = std::strtoull(argv[1], nullptr, 10);
    std::size_t pairs = std::strtoull(argv[2], nullptr, 10);
    if (batches == 0 || pairs == 0) { return 1; }
    std::mutex io_mutex;
    std::unique_ptr< producer_consumer_pair[] > states(new producer_consumer_pair[pairs]);
    for (std::size_t identity = 0; identity < pairs; ++identity)
    {
        states[identity].identity = identity;
        states[identity].batches = batches;
        states[identity].io_mutex = &io_mutex;
    }
    std::vector< std::thread > workers;
    workers.reserve(pairs * 2);
    std::uint64_t start = ALLOCATION_TICKS();
    for (std::size_t identity = 0; identity < pairs; ++identity)
    {
        producer_consumer_pair* state = &states[identity];
        workers.emplace_back([state] { state->produce(); });
        workers.emplace_back([state] { state->consume(); });
    }
    for (std::thread& worker : workers) { worker.join(); }
    std::uint64_t elapsed = ALLOCATION_TICKS() - start;
    std::printf("%llu %llu\n", static_cast< unsigned long long >(elapsed),
                static_cast< unsigned long long >(ALLOCATION_FREQUENCY()));
}
