#ifndef QUXLANG_BENCHMARK_ALLOCATOR_BARRIERS_H
#define QUXLANG_BENCHMARK_ALLOCATOR_BARRIERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Exposes an allocation to an opaque assembly procedure without accessing it. */
void DO_NOT_OPTIMIZE(void* pointer);

/** Writes one byte at the allocation address through an opaque assembly procedure. */
void ALLOCATION_WRITE_BYTE(void* pointer);

/** Reads the architecture timer with instruction ordering barriers. */
uint64_t ALLOCATION_TICKS(void);

/** Returns the architecture timer frequency in ticks per second. */
uint64_t ALLOCATION_FREQUENCY(void);

#ifdef __cplusplus
}
#endif

#endif
