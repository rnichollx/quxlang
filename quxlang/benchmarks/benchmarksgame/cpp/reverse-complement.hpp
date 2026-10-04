#include <cstdint>
#ifndef QUXLANG_BENCHMARKSGAME_CPP_REVERSE_COMPLEMENT_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_REVERSE_COMPLEMENT_HPP
#include "output.hpp"
#include <span>

/** Emits one complete FASTA record in reverse-complement order. */
void write_complement(std::string_view header, std::span< std::uint8_t const > sequence, std::array< std::uint8_t, 256 > const& complements, output_buffer& output);

#endif
