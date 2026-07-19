// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_BENCHMARKS_VARIANT_BMS_HEADER_GUARD
#define QUXLANG_SOURCES_BENCHMARKS_VARIANT_BMS_HEADER_GUARD

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

/** Storage-padded benchmark alternative. */
template < std::size_t N, typename Int >
struct padded;
/** Zero-padding specialization of the benchmark alternative. */
template < typename Int >
struct padded< 0, Int >;

/** Reads the numeric payload of a padded benchmark alternative. */
struct visitor_uint64;

extern std::size_t const max_bm_size;

/** Generates a deterministic array of benchmark variants. */
template < typename Variant >
auto GenerateVariantArray(std::size_t count) -> std::vector< Variant >;
/** Runs one configured variant visitation benchmark. */
template < typename Variant >
void RunBM_Variant(benchmark::State& state);
/** Dispatches the variant benchmark by alternative count. */
void BM_Variant(benchmark::State& state);
/** Registers argument combinations for the variant benchmark. */
void VariantArgs(benchmark::internal::Benchmark* benchmark);
/** Benchmarks expression formatting. */
void BM_SomeFunction(benchmark::State& state);

#endif // QUXLANG_SOURCES_BENCHMARKS_VARIANT_BMS_HEADER_GUARD
