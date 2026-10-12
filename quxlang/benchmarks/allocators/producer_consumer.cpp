#include "barriers.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <mimalloc-new-delete.h>

#if defined(__APPLE__)
#include <os/os_sync_wait_on_address.h>
#elif defined(__linux__)
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#error The producer-consumer benchmark requires native address waiting
#endif

static_assert(std::atomic< std::int32_t >::is_always_lock_free);
static_assert(sizeof(std::atomic< std::int32_t >) == sizeof(std::int32_t));
static_assert(alignof(std::atomic< std::int32_t >) >= sizeof(std::int32_t));

/** Compares the complete atomic word before sleeping; spurious returns are permitted. */
static void wait_while_atomic_equals(std::atomic< std::int32_t >& state, std::int32_t expected)
{
#if defined(__APPLE__)
    os_sync_wait_on_address(&state, static_cast< std::uint32_t >(expected), sizeof(expected),
                            OS_SYNC_WAIT_ON_ADDRESS_NONE);
#elif defined(__linux__)
    syscall(SYS_futex, &state, FUTEX_WAIT, expected, nullptr, nullptr, 0);
#elif defined(_WIN32)
    WaitOnAddress(&state, &expected, sizeof(expected), INFINITE);
#endif
}

/** Wakes one thread waiting on the atomic word after its value has been published. */
static void wake_one_atomic_waiter(std::atomic< std::int32_t >& state)
{
#if defined(__APPLE__)
    os_sync_wake_by_address_any(&state, sizeof(std::int32_t), OS_SYNC_WAKE_BY_ADDRESS_NONE);
#elif defined(__linux__)
    syscall(SYS_futex, &state, FUTEX_WAKE, 1, nullptr, nullptr, 0);
#elif defined(_WIN32)
    WakeByAddressSingle(&state);
#endif
}

/** Matches the Quxlang runtime mutex state transitions, memory ordering, and address waits. */
class unfair_mutex
{
    // Zero is unlocked, one is locked, and two records a wake obligation.
    std::atomic< std::int32_t > state{0};

public:
    /** Acquires ownership, registering contention before sleeping. */
    void lock()
    {
        std::int32_t expected = 0;
        if (state.compare_exchange_strong(expected, 1, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            return;
        }
        for (;;)
        {
            std::int32_t expected_uncontended = 1;
            state.compare_exchange_strong(expected_uncontended, 2,
                                          std::memory_order_acq_rel, std::memory_order_acquire);
            std::int32_t observed = state.load(std::memory_order_acquire);
            if (observed == 0)
            {
                std::int32_t expected_unlocked = 0;
                if (state.compare_exchange_strong(expected_unlocked, 2,
                                                  std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return;
                }
            }
            else if (observed == 2)
            {
                wait_while_atomic_equals(state, 2);
            }
        }
    }

    /** Releases ownership and wakes one contender when required. */
    void unlock()
    {
        std::int32_t expected_uncontended = 1;
        if (state.compare_exchange_strong(expected_uncontended, 0,
                                          std::memory_order_acq_rel, std::memory_order_acquire))
        {
            return;
        }
        assert(expected_uncontended == 2);
        state.store(0, std::memory_order_release);
        wake_one_atomic_waiter(state);
    }
};

/** Matches the Quxlang condition variable notification sequence and waiter registration. */
class condition_variable
{
    std::atomic< std::int32_t > sequence{0};
    std::atomic< std::uint32_t > waiters{0};

public:
    /** Releases the owned mutex while waiting and reacquires it before returning. */
    void wait(unfair_mutex& locked_mutex)
    {
        std::int32_t observed = sequence.load(std::memory_order_acquire);
        waiters.fetch_add(1, std::memory_order_release);
        locked_mutex.unlock();
        wait_while_atomic_equals(sequence, observed);
        waiters.fetch_sub(1, std::memory_order_relaxed);
        locked_mutex.lock();
    }

    /** Advances the sequence and wakes one registered waiter. */
    void notify_one()
    {
        sequence.fetch_add(1, std::memory_order_release);
        if (waiters.load(std::memory_order_acquire) == 0) { return; }
        wake_one_atomic_waiter(sequence);
    }
};

/** A bounded FIFO of borrowed pointers, including null termination markers. */
template < typename T, std::size_t Capacity >
class synchronized_pointer_queue
{
    static_assert(Capacity > 0);
    std::array< T*, Capacity > entries{};
    std::size_t head = 0;
    std::size_t count = 0;
    unfair_mutex mutex;
    condition_variable readable;
    condition_variable writable;

public:
    /** Publishes a pointer after waiting for an available slot. */
    void push(T* value)
    {
        std::unique_lock< unfair_mutex > lock(mutex);
        while (count == Capacity) { writable.wait(mutex); }
        entries[(head + count) % Capacity] = value;
        ++count;
        readable.notify_one();
    }

    /** Removes the oldest pointer after waiting for an entry. */
    T* pop()
    {
        std::unique_lock< unfair_mutex > lock(mutex);
        while (count == 0) { readable.wait(mutex); }
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
    unfair_mutex* io_mutex = nullptr;

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
        std::lock_guard< unfair_mutex > lock(*io_mutex);
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
    unfair_mutex io_mutex;
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
