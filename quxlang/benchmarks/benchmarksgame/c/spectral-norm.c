#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/** Returns one entry of the infinite matrix. */
double matrix_element(size_t row, size_t column)
{
    size_t diagonal = row + column;
    return 1.0 / (double)(diagonal * (diagonal + 1) / 2 + row + 1);
}

/** Multiplies the leading square matrix by a vector. */
void multiply_a(size_t n, const double* input, double* output)
{
    for (size_t row = 0; row < n; ++row)
    {
        double sum = 0;
        for (size_t column = 0; column < n; ++column)
        {
            sum += matrix_element(row, column) * input[column];
        }
        output[row] = sum;
    }
}

/** Multiplies the transpose by a vector. */
void multiply_transpose(size_t n, const double* input, double* output)
{
    for (size_t row = 0; row < n; ++row)
    {
        double sum = 0;
        for (size_t column = 0; column < n; ++column)
        {
            sum += matrix_element(column, row) * input[column];
        }
        output[row] = sum;
    }
}

/** Applies A followed by its transpose using reusable scratch storage. */
void multiply_ata(size_t n, const double* input, double* output, double* scratch)
{
    multiply_a(n, input, scratch);
    multiply_transpose(n, scratch, output);
}

/** Computes the spectral norm by ten pairs of power iterations. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    size_t n = strtoul(argv[1], NULL, 10);
    if (n == 0 || n > 100000)
    {
        return 2;
    }
    double* u = calloc(n, sizeof(double));
    double* v = calloc(n, sizeof(double));
    double* scratch = calloc(n, sizeof(double));
    if (!u || !v || !scratch)
    {
        abort();
    }
    for (size_t i = 0; i < n; ++i)
    {
        u[i] = 1;
    }
    for (unsigned iteration = 0; iteration < 10; ++iteration)
    {
        multiply_ata(n, u, v, scratch);
        multiply_ata(n, v, u, scratch);
    }
    double uv = 0;
    double vv = 0;
    for (size_t i = 0; i < n; ++i)
    {
        uv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    printf("%.9f\n", sqrt(uv / vv));
    free(scratch);
    free(v);
    free(u);
    return 0;
}
