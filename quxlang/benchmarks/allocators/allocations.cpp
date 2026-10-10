#include "barriers.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <new>
#include <thread>

#if defined(USE_MIMALLOC)
#include <mimalloc-new-delete.h>
#endif

/** Owns the size-specific allocation and observation procedure. */
template < std::size_t Bytes >
struct allocation_workload
{
    /** Uninitialized payload with the same alignment as the Quxlang object. */
    struct block
    {
        std::uint64_t words[Bytes / 8];
    };

    /** Allocates each batch, observes every pointer, and releases the batch in FIFO order. */
    static void allocate_batches(std::size_t n, std::size_t batch)
    {
        block* pointers[20];
        for (std::size_t completed = 0; completed < n;)
        {
            std::size_t count = batch < n - completed ? batch : n - completed;
            for (std::size_t index = 0; index < count; ++index)
            {
#if defined(USE_MALLOC)
                void* storage = std::malloc(sizeof(block));
                if (storage == nullptr)
                {
                    std::abort();
                }
                block* pointer = new (storage) block;
#else
                block* pointer = new block;
#endif
                pointers[index] = pointer;
            }
            for (std::size_t index = 0; index < count; ++index)
            {
                DO_NOT_OPTIMIZE(pointers[index]);
            }
            for (std::size_t index = 0; index < count; ++index)
            {
#if defined(USE_MALLOC)
                pointers[index]->~block();
                std::free(pointers[index]);
#else
                delete pointers[index];
#endif
            }
            completed += count;
        }
    }

    /** Measures batches using the same worker release protocol as Quxlang. */
    static void run(std::size_t n, std::size_t batch, std::size_t threads)
    {
        std::uint64_t elapsed;
        if (threads == 1)
        {
            std::uint64_t start = ALLOCATION_TICKS();
            allocate_batches(n, batch);
            elapsed = ALLOCATION_TICKS() - start;
        }
        else
        {
            std::mutex mutex;
            std::condition_variable condition;
            std::condition_variable progress;
            std::size_t ready = 0;
            std::size_t finished = 0;
            bool start = false;
            bool release = false;
            std::uint64_t last_finish = 0;
            std::thread workers[32];
            for (std::thread& worker : workers)
            {
                worker = std::thread([&]() {
                    allocate_batches(10000, batch);
                    std::unique_lock< std::mutex > lock(mutex);
                    ++ready;
                    progress.notify_one();
                    condition.wait(lock, [&]() { return start; });
                    lock.unlock();
                    allocate_batches(n, batch);
                    std::uint64_t finish = ALLOCATION_TICKS();
                    lock.lock();
                    if (finish > last_finish) { last_finish = finish; }
                    ++finished;
                    progress.notify_one();
                    condition.wait(lock, [&]() { return release; });
                });
            }
            std::unique_lock< std::mutex > lock(mutex);
            progress.wait(lock, [&]() { return ready == 32; });
            std::uint64_t begin = ALLOCATION_TICKS();
            start = true;
            condition.notify_all();
            progress.wait(lock, [&]() { return finished == 32; });
            elapsed = last_finish - begin;
            release = true;
            condition.notify_all();
            lock.unlock();
            for (std::thread& worker : workers) { worker.join(); }
        }
        std::printf("%llu %llu\n", static_cast< unsigned long long >(elapsed),
                    static_cast< unsigned long long >(ALLOCATION_FREQUENCY()));
    }
};

/** Selects a statically sized object and the requested lifetime pattern. */
int main(int argc, char** argv)
{
    if (argc != 5)
    {
        return 2;
    }
    std::size_t n = std::strtoull(argv[1], nullptr, 10);
    std::size_t size = std::strtoull(argv[2], nullptr, 10);
    std::size_t batch = std::strtoull(argv[3], nullptr, 10);
    std::size_t threads = std::strtoull(argv[4], nullptr, 10);
    if (n == 0 || batch == 0 || batch > 20 || (threads != 1 && threads != 32))
    {
        return 2;
    }
    switch (size)
    {
    case 8:
        allocation_workload< 8 >::run(n, batch, threads);
        break;
    case 16:
        allocation_workload< 16 >::run(n, batch, threads);
        break;
    case 24:
        allocation_workload< 24 >::run(n, batch, threads);
        break;
    case 32:
        allocation_workload< 32 >::run(n, batch, threads);
        break;
    case 64:
        allocation_workload< 64 >::run(n, batch, threads);
        break;
    default:
        return 2;
    }
}
