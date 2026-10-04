#ifndef QUXLANG_BENCHMARKSGAME_K_NUCLEOTIDE_HPP
#define QUXLANG_BENCHMARKSGAME_K_NUCLEOTIDE_HPP

#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>

/** Accumulates k-mer counts from one reading frame. */
void accumulate_frame(std::span< std::uint8_t const > sequence, std::size_t width, std::size_t frame, std::unordered_map< std::uint64_t, std::uint64_t >& counts);
/** Counts all reading frames in a fresh standard-library hash table. */
std::unordered_map< std::uint64_t, std::uint64_t > count_nucleotides(std::span< std::uint8_t const > sequence, std::size_t width);
/** Writes frequencies sorted by descending count and then ascending key. */
void write_frequencies(std::span< std::uint8_t const > sequence, std::size_t width);
/** Writes one requested sequence's count after counting every sequence of that width. */
void write_count(std::span< std::uint8_t const > sequence, std::string_view query);

#endif
