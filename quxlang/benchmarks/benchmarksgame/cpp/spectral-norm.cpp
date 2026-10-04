#include "spectral-norm.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

double matrix_element(std::size_t row, std::size_t column)
{
    std::size_t diagonal = row + column;
    return 1.0 / static_cast< double >(diagonal * (diagonal + 1) / 2 + row + 1);
}

void multiply_a(std::vector< double > const& input, std::vector< double >& output)
{
    for (std::size_t row = 0; row < input.size(); ++row)
    {
        double sum = 0;
        for (std::size_t column = 0; column < input.size(); ++column)
        {
            sum += matrix_element(row, column) * input[column];
        }
        output[row] = sum;
    }
}

void multiply_transpose(std::vector< double > const& input, std::vector< double >& output)
{
    for (std::size_t row = 0; row < input.size(); ++row)
    {
        double sum = 0;
        for (std::size_t column = 0; column < input.size(); ++column)
        {
            sum += matrix_element(column, row) * input[column];
        }
        output[row] = sum;
    }
}

void multiply_ata(std::vector< double > const& input, std::vector< double >& output, std::vector< double >& scratch)
{
    multiply_a(input, scratch);
    multiply_transpose(scratch, output);
}

/** Computes the spectral norm by ten pairs of power iterations. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    std::size_t n = std::strtoul(argv[1], nullptr, 10);
    if (n == 0 || n > 100000)
    {
        return 2;
    }
    std::vector< double > u(n, 1);
    std::vector< double > v(n);
    std::vector< double > scratch(n);
    for (std::uint32_t iteration = 0; iteration < 10; ++iteration)
    {
        multiply_ata(u, v, scratch);
        multiply_ata(v, u, scratch);
    }
    double uv = 0;
    double vv = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        uv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    std::printf("%.9f\n", std::sqrt(uv / vv));
}
