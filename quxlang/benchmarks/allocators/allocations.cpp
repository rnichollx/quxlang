#include "barriers.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <new>

#if defined(USE_MIMALLOC)
#include <mimalloc-new-delete.h>
#endif

/** Owns the size-specific allocation and observation procedure. */
template < std::size_t Bytes >
struct allocation_workload
{
    /** Zero-initialized payload with the same alignment as the Quxlang object. */
    struct block
    {
        std::uint64_t words[Bytes / 8];
    };

    /** Returns monotonic nanoseconds. */
    static std::uint64_t nanoseconds()
    {
        timespec value;
        if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
        {
            std::abort();
        }
        return static_cast< std::uint64_t >(value.tv_sec) * 1000000000 + value.tv_nsec;
    }

    /** Measures allocation and FIFO release with opaque pointer and memory barriers. */
    static void run(std::size_t n, std::size_t batch)
    {
        block* pointers[1024];
        std::uint64_t start = nanoseconds();
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
                block* pointer = new (storage) block{};
#else
                block* pointer = new block{};
#endif
                DO_NOT_OPTIMIZE(pointer);
                pointers[index] = pointer;
            }
            CLOBBER_MEMORY();
            for (std::size_t index = 0; index < count; ++index)
            {
#if defined(USE_MALLOC)
                pointers[index]->~block();
                std::free(pointers[index]);
#else
                delete pointers[index];
#endif
            }
            CLOBBER_MEMORY();
            completed += count;
        }
        std::uint64_t elapsed = nanoseconds() - start;
        std::printf("%llu\n", static_cast< unsigned long long >(elapsed));
    }
};

/** Selects a statically sized object and the requested lifetime pattern. */
int main(int argc, char** argv)
{
    if (argc != 4)
    {
        return 2;
    }
    std::size_t n = std::strtoull(argv[1], nullptr, 10);
    std::size_t size = std::strtoull(argv[2], nullptr, 10);
    std::size_t batch = std::strtoull(argv[3], nullptr, 10);
    if (n == 0 || batch == 0 || batch > 1024)
    {
        return 2;
    }
    switch (size)
    {
    case 16:
        allocation_workload< 16 >::run(n, batch);
        break;
    case 24:
        allocation_workload< 24 >::run(n, batch);
        break;
    case 32:
        allocation_workload< 32 >::run(n, batch);
        break;
    case 48:
        allocation_workload< 48 >::run(n, batch);
        break;
    case 64:
        allocation_workload< 64 >::run(n, batch);
        break;
    case 96:
        allocation_workload< 96 >::run(n, batch);
        break;
    case 128:
        allocation_workload< 128 >::run(n, batch);
        break;
    case 256:
        allocation_workload< 256 >::run(n, batch);
        break;
    default:
        return 2;
    }
}
