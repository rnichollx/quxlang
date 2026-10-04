#ifndef QUXLANG_BENCHMARK_ALLOCATOR_BARRIERS_H
#define QUXLANG_BENCHMARK_ALLOCATOR_BARRIERS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Exposes an allocation to an opaque assembly procedure without accessing it. */
void DO_NOT_OPTIMIZE(void* pointer);

/** Prevents memory operations from moving across allocation phases. */
void CLOBBER_MEMORY(void);

#ifdef __cplusplus
}
#endif

#endif
