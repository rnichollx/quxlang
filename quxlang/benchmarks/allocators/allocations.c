#define _POSIX_C_SOURCE 200809L
#include "barriers.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(USE_MIMALLOC)
#include <mimalloc-override.h>
#endif

/** Returns monotonic nanoseconds for the measured allocation loop. */
static uint64_t allocation_nanoseconds(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
    {
        abort();
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec;
}

/** Allocates and releases in FIFO order, retaining at most batch live objects. */
static void allocate_batches(size_t n, size_t size, size_t batch)
{
    uint64_t* pointers[1024];
    for (size_t completed = 0; completed < n;)
    {
        size_t count = batch < n - completed ? batch : n - completed;
        for (size_t index = 0; index < count; ++index)
        {
            uint64_t* pointer = malloc(size);
            if (pointer == NULL)
            {
                abort();
            }
            memset(pointer, 0, size);
            DO_NOT_OPTIMIZE(pointer);
            pointers[index] = pointer;
        }
        CLOBBER_MEMORY();
        for (size_t index = 0; index < count; ++index)
        {
            free(pointers[index]);
        }
        CLOBBER_MEMORY();
        completed += count;
    }
}

/** Coordinates warmed workers and holds thread teardown until timing ends. */
struct allocation_run
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    pthread_cond_t progress;
    size_t n;
    size_t size;
    size_t batch;
    size_t ready;
    size_t finished;
    bool start;
    bool release;
    uint64_t last_finish;
};

/** Terminates the benchmark when native synchronization fails. */
static void check_pthread(int result)
{
    if (result != 0)
    {
        abort();
    }
}

/** Warms one allocator, waits for release, and records completion before teardown. */
static void* allocation_worker(void* argument)
{
    struct allocation_run* run = argument;
    allocate_batches(10000, run->size, run->batch);
    check_pthread(pthread_mutex_lock(&run->mutex));
    ++run->ready;
    check_pthread(pthread_cond_signal(&run->progress));
    while (!run->start)
    {
        check_pthread(pthread_cond_wait(&run->condition, &run->mutex));
    }
    check_pthread(pthread_mutex_unlock(&run->mutex));
    allocate_batches(run->n, run->size, run->batch);
    uint64_t finish = allocation_nanoseconds();
    check_pthread(pthread_mutex_lock(&run->mutex));
    if (finish > run->last_finish)
    {
        run->last_finish = finish;
    }
    ++run->finished;
    check_pthread(pthread_cond_signal(&run->progress));
    while (!run->release)
    {
        check_pthread(pthread_cond_wait(&run->condition, &run->mutex));
    }
    check_pthread(pthread_mutex_unlock(&run->mutex));
    return NULL;
}

/** Measures 32 workers from start release through the last allocation batch. */
static uint64_t run_parallel(size_t n, size_t size, size_t batch)
{
    struct allocation_run run = {.n = n, .size = size, .batch = batch};
    check_pthread(pthread_mutex_init(&run.mutex, NULL));
    check_pthread(pthread_cond_init(&run.condition, NULL));
    check_pthread(pthread_cond_init(&run.progress, NULL));
    pthread_t workers[32];
    for (size_t index = 0; index < 32; ++index)
    {
        check_pthread(pthread_create(&workers[index], NULL, allocation_worker, &run));
    }
    check_pthread(pthread_mutex_lock(&run.mutex));
    while (run.ready != 32)
    {
        check_pthread(pthread_cond_wait(&run.progress, &run.mutex));
    }
    uint64_t start = allocation_nanoseconds();
    run.start = true;
    check_pthread(pthread_cond_broadcast(&run.condition));
    while (run.finished != 32)
    {
        check_pthread(pthread_cond_wait(&run.progress, &run.mutex));
    }
    uint64_t elapsed = run.last_finish - start;
    run.release = true;
    check_pthread(pthread_cond_broadcast(&run.condition));
    check_pthread(pthread_mutex_unlock(&run.mutex));
    for (size_t index = 0; index < 32; ++index)
    {
        check_pthread(pthread_join(workers[index], NULL));
    }
    check_pthread(pthread_cond_destroy(&run.condition));
    check_pthread(pthread_cond_destroy(&run.progress));
    check_pthread(pthread_mutex_destroy(&run.mutex));
    return elapsed;
}

/** Selects object size, allocations per worker, batch size, and thread count. */
int main(int argc, char** argv)
{
    if (argc != 5)
    {
        return 2;
    }
    size_t n = strtoull(argv[1], NULL, 10);
    size_t size = strtoull(argv[2], NULL, 10);
    size_t batch = strtoull(argv[3], NULL, 10);
    size_t threads = strtoull(argv[4], NULL, 10);
    if (n == 0 || batch == 0 || batch > 1024 || (threads != 1 && threads != 32) || (size != 16 && size != 24 && size != 32 && size != 48 && size != 64 && size != 96 && size != 128 && size != 256))
    {
        return 2;
    }
    uint64_t elapsed;
    if (threads == 32)
    {
        elapsed = run_parallel(n, size, batch);
    }
    else
    {
        uint64_t start = allocation_nanoseconds();
        allocate_batches(n, size, batch);
        elapsed = allocation_nanoseconds() - start;
    }
    printf("%" PRIu64 "\n", elapsed);
    return 0;
}
