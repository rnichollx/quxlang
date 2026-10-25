#include "barriers.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

#if defined(USE_MIMALLOC)
#include <mimalloc-new-delete.h>
#endif

/** The workload owns the size-specific allocation and observation procedure. */
template < std::size_t Bytes, std::size_t Pattern >
struct lifetime_workload
{
    /** The block contains uninitialized payload with the same alignment as the Quxlang object. */
    struct block
    {
        std::uint64_t words[Bytes / 8];
    };

    /** The allocation loop returns the payload-word sum after exactly n allocation/free pairs. */
    static std::uint64_t allocate_batches(std::size_t n, std::size_t batch, std::vector< block* >& pointers)
    {
        std::uint64_t checksum = 0;
        if constexpr (Pattern == 2)
        {
            std::size_t count = batch < n ? batch : n;
            for (std::size_t index = 0; index < count; ++index)
            {
                pointers[index] = new block;
                ALLOCATION_WRITE_WORD(pointers[index], index + 1);
            }
            std::size_t position = 0;
            for (std::size_t completed = count; completed < n; ++completed)
            {
                checksum += *reinterpret_cast< std::uint64_t* >(pointers[position]);
                delete pointers[position];
                pointers[position] = new block;
                ALLOCATION_WRITE_WORD(pointers[position], completed + 1);
                if (++position == count) { position = 0; }
            }
            for (std::size_t index = 0; index < count; ++index)
            {
                checksum += *reinterpret_cast< std::uint64_t* >(pointers[index]);
                delete pointers[index];
            }
        }
        else
        {
            for (std::size_t completed = 0; completed < n;)
            {
                std::size_t count = batch < n - completed ? batch : n - completed;
                for (std::size_t index = 0; index < count; ++index)
                {
                    pointers[index] = new block;
                    ALLOCATION_WRITE_WORD(pointers[index], completed + index + 1);
                }
                for (std::size_t index = 0; index < count; ++index)
                {
                    std::size_t slot = Pattern == 1 ? count - 1 - index : index;
                    checksum += *reinterpret_cast< std::uint64_t* >(pointers[slot]);
                    delete pointers[slot];
                }
                completed += count;
            }
        }
        return checksum;
    }

    /** The driver measures batches using the same worker release protocol as Quxlang. */
    static void run(std::size_t n, std::size_t batch, std::size_t threads)
    {
        std::uint64_t elapsed;
        std::uint64_t checksum = 0;
        if (threads == 1)
        {
            std::vector< block* > pointers(batch);
            std::uint64_t start = ALLOCATION_TICKS();
            checksum = allocate_batches(n, batch, pointers);
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
            std::vector< std::thread > workers(threads);
            for (std::thread& worker : workers)
            {
                worker = std::thread([&]() {
                    std::vector< block* > pointers(batch);
                    allocate_batches(10000, batch, pointers);
                    std::unique_lock< std::mutex > lock(mutex);
                    ++ready;
                    progress.notify_one();
                    condition.wait(lock, [&]() { return start; });
                    lock.unlock();
                    std::uint64_t worker_checksum = allocate_batches(n, batch, pointers);
                    std::uint64_t finish = ALLOCATION_TICKS();
                    lock.lock();
                    if (finish > last_finish) { last_finish = finish; }
                    checksum += worker_checksum;
                    ++finished;
                    progress.notify_one();
                    condition.wait(lock, [&]() { return release; });
                });
            }
            std::unique_lock< std::mutex > lock(mutex);
            progress.wait(lock, [&]() { return ready == threads; });
            std::uint64_t begin = ALLOCATION_TICKS();
            start = true;
            condition.notify_all();
            progress.wait(lock, [&]() { return finished == threads; });
            elapsed = last_finish - begin;
            release = true;
            condition.notify_all();
            lock.unlock();
            for (std::thread& worker : workers) { worker.join(); }
        }
        std::printf("%llu %llu %llu\n", static_cast< unsigned long long >(elapsed),
                    static_cast< unsigned long long >(ALLOCATION_FREQUENCY()), static_cast< unsigned long long >(checksum));
    }
};

/** The entry point selects the matched object size and lifetime policy. */
int main(int argc, char** argv)
{
    if (argc != 6) { return 2; }
    std::size_t n = std::strtoull(argv[1], nullptr, 10);
    std::size_t size = std::strtoull(argv[2], nullptr, 10);
    std::size_t batch = std::strtoull(argv[3], nullptr, 10);
    std::size_t threads = std::strtoull(argv[4], nullptr, 10);
    std::size_t pattern = std::strtoull(argv[5], nullptr, 10);
    if (n == 0 || batch == 0 || batch > 4096 || threads == 0 || threads > 128 || pattern > 2) { return 2; }
    if (size == 16 && pattern == 0) { lifetime_workload< 16, 0 >::run(n, batch, threads); return 0; }
    if (size == 16 && pattern == 1) { lifetime_workload< 16, 1 >::run(n, batch, threads); return 0; }
    if (size == 16 && pattern == 2) { lifetime_workload< 16, 2 >::run(n, batch, threads); return 0; }
    if (size == 64 && pattern == 0) { lifetime_workload< 64, 0 >::run(n, batch, threads); return 0; }
    if (size == 64 && pattern == 1) { lifetime_workload< 64, 1 >::run(n, batch, threads); return 0; }
    if (size == 64 && pattern == 2) { lifetime_workload< 64, 2 >::run(n, batch, threads); return 0; }
    if (size == 256 && pattern == 0) { lifetime_workload< 256, 0 >::run(n, batch, threads); return 0; }
    if (size == 256 && pattern == 1) { lifetime_workload< 256, 1 >::run(n, batch, threads); return 0; }
    if (size == 256 && pattern == 2) { lifetime_workload< 256, 2 >::run(n, batch, threads); return 0; }
    if (size == 1024 && pattern == 0) { lifetime_workload< 1024, 0 >::run(n, batch, threads); return 0; }
    if (size == 1024 && pattern == 1) { lifetime_workload< 1024, 1 >::run(n, batch, threads); return 0; }
    if (size == 1024 && pattern == 2) { lifetime_workload< 1024, 2 >::run(n, batch, threads); return 0; }
    if (size == 4096 && pattern == 0) { lifetime_workload< 4096, 0 >::run(n, batch, threads); return 0; }
    if (size == 4096 && pattern == 1) { lifetime_workload< 4096, 1 >::run(n, batch, threads); return 0; }
    if (size == 4096 && pattern == 2) { lifetime_workload< 4096, 2 >::run(n, batch, threads); return 0; }
    if (size == 8192 && pattern == 0) { lifetime_workload< 8192, 0 >::run(n, batch, threads); return 0; }
    if (size == 8192 && pattern == 1) { lifetime_workload< 8192, 1 >::run(n, batch, threads); return 0; }
    if (size == 8192 && pattern == 2) { lifetime_workload< 8192, 2 >::run(n, batch, threads); return 0; }
    return 2;
}
