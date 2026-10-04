#ifndef QUXLANG_BENCHMARKSGAME_CPP_SPECTRAL_NORM_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_SPECTRAL_NORM_HPP
#include <cstddef>
#include <vector>

/** Returns one entry of the infinite matrix. */
double matrix_element(std::size_t row, std::size_t column);

/** Multiplies the leading square matrix by a vector. */
void multiply_a(std::vector< double > const& input, std::vector< double >& output);

/** Multiplies the transpose by a vector. */
void multiply_transpose(std::vector< double > const& input, std::vector< double >& output);

/** Applies A followed by its transpose using reusable scratch storage. */
void multiply_ata(std::vector< double > const& input, std::vector< double >& output, std::vector< double >& scratch);

#endif
