#ifndef QUXLANG_BENCHMARKSGAME_CPP_FASTA_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_FASTA_HPP
#include "output.hpp"
#include <cstdint>
#include <span>
#include <string_view>

/** Generates a weighted sequence with the prescribed LCG and cumulative probabilities. */
void random_sequence(std::string_view letters, std::span< double const > probabilities, std::uint32_t& seed, std::size_t length, output_buffer& output);

#endif
