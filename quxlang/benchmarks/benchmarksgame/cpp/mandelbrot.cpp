#include "output.hpp"
#include <cstdint>

/** Writes the prescribed Mandelbrot bitmap using fifty scalar iterations per pixel. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    std::size_t n = std::strtoul(argv[1], nullptr, 10);
    if (n == 0)
    {
        return 2;
    }
    std::printf("P4\n%zu %zu\n", n, n);
    output_buffer output;
    for (std::size_t y = 0; y < n; ++y)
    {
        double ci = 2.0 * static_cast< double >(y) / static_cast< double >(n) - 1.0;
        std::uint8_t byte = 0;
        std::uint32_t bits = 0;
        for (std::size_t x = 0; x < n; ++x)
        {
            double cr = 2.0 * static_cast< double >(x) / static_cast< double >(n) - 1.5;
            double zr = 0, zi = 0, tr = 0, ti = 0;
            std::uint8_t inside = 1;
            for (std::uint32_t iteration = 0; iteration < 50; ++iteration)
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
            byte = static_cast< std::uint8_t >((byte << 1) | inside);
            if (++bits == 8)
            {
                output.write_byte(byte);
                byte = 0;
                bits = 0;
            }
        }
        if (bits != 0)
        {
            output.write_byte(static_cast< std::uint8_t >(byte << (8 - bits)));
        }
    }
    output.flush();
    return 0;
}
