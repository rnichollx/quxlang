#include "output.h"

/** Writes the prescribed Mandelbrot bitmap using fifty scalar iterations per pixel. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    size_t n = strtoul(argv[1], NULL, 10);
    if (n == 0)
    {
        return 2;
    }
    printf("P4\n%zu %zu\n", n, n);
    struct output_buffer output = {{0}, 0};
    for (size_t y = 0; y < n; ++y)
    {
        double ci = 2.0 * (double)y / (double)n - 1.0;
        unsigned char byte = 0;
        unsigned bits = 0;
        for (size_t x = 0; x < n; ++x)
        {
            double cr = 2.0 * (double)x / (double)n - 1.5;
            double zr = 0, zi = 0, tr = 0, ti = 0;
            unsigned char inside = 1;
            for (unsigned iteration = 0; iteration < 50; ++iteration)
            {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
                if (tr + ti > 4.0)
                {
                    inside = 0;
                    break;
                }
            }
            byte = (unsigned char)((byte << 1) | inside);
            if (++bits == 8)
            {
                write_byte(&output, byte);
                byte = 0;
                bits = 0;
            }
        }
        if (bits != 0)
        {
            write_byte(&output, (unsigned char)(byte << (8 - bits)));
        }
    }
    flush_output(&output);
    return 0;
}
