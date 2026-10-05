#include <cstdint>
#ifndef QUXLANG_BENCHMARKSGAME_CPP_REVERSE_COMPLEMENT_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_REVERSE_COMPLEMENT_HPP
#include "output.hpp"
#include <span>
#include <streambuf>

/** Reads standard input in blocks for line-oriented FASTA parsing. */
class reverse_complement_input_buffer : public std::streambuf
{
    std::array< char, 16384 > bytes_{};

  protected:
    /** Refills the input area and reports end of input or a read failure. */
    int_type underflow() override;
};

/** Emits one complete FASTA record in reverse-complement order. */
void write_complement(std::string_view header, std::span< std::uint8_t const > sequence, std::array< std::uint8_t, 256 > const& complements, output_buffer& output);

#endif
