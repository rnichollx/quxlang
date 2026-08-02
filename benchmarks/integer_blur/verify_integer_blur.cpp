#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace integer_blur_verifier
{
    constexpr std::size_t file_header_size = 16;
    constexpr std::size_t convolution_iteration_count = 256;
    constexpr std::uint32_t maximum_dimension = 65'536;
    constexpr std::uint8_t file_magic[8] = {'Q', 'I', 'M', 'G', '1', '6', 'L', 'E'};

    struct image
    {
        std::uint32_t width;
        std::uint32_t height;
        std::vector< std::uint16_t > pixels;
    };

    /** Decodes one little-endian 32-bit integer. */
    auto decode_u32(std::uint8_t const* bytes) -> std::uint32_t
    {
        return static_cast< std::uint32_t >(bytes[0]) |
               (static_cast< std::uint32_t >(bytes[1]) << 8U) |
               (static_cast< std::uint32_t >(bytes[2]) << 16U) |
               (static_cast< std::uint32_t >(bytes[3]) << 24U);
    }

    /** Loads and validates one complete QIMG16LE file. */
    auto load_image(std::string_view path) -> image
    {
        std::ifstream input(std::string(path), std::ios::binary | std::ios::ate);
        if (!input)
        {
            throw std::runtime_error("could not open image");
        }
        std::streamoff stream_size = input.tellg();
        if (stream_size < static_cast< std::streamoff >(file_header_size))
        {
            throw std::runtime_error("image is shorter than its header");
        }
        if (static_cast< std::uintmax_t >(stream_size) > std::numeric_limits< std::size_t >::max())
        {
            throw std::runtime_error("image is too large for this process");
        }

        std::vector< std::uint8_t > bytes(static_cast< std::size_t >(stream_size));
        input.seekg(0);
        input.read(reinterpret_cast< char* >(bytes.data()), stream_size);
        if (!input)
        {
            throw std::runtime_error("could not read image");
        }
        for (std::size_t index = 0; index != 8; ++index)
        {
            if (bytes[index] != file_magic[index])
            {
                throw std::runtime_error("image has an invalid QIMG16LE header");
            }
        }

        std::uint32_t width = decode_u32(bytes.data() + 8);
        std::uint32_t height = decode_u32(bytes.data() + 12);
        if (width < 7 || height < 7 || width > maximum_dimension || height > maximum_dimension)
        {
            throw std::runtime_error("image dimensions are outside the supported range");
        }
        std::size_t pixel_count = static_cast< std::size_t >(width) * static_cast< std::size_t >(height);
        if (pixel_count > (std::numeric_limits< std::size_t >::max() - file_header_size) / 2 ||
            bytes.size() != file_header_size + pixel_count * 2)
        {
            throw std::runtime_error("image size does not match its dimensions");
        }

        std::vector< std::uint16_t > pixels(pixel_count);
        for (std::size_t index = 0; index != pixel_count; ++index)
        {
            std::size_t offset = file_header_size + index * 2;
            pixels[index] = static_cast< std::uint16_t >(
                static_cast< std::uint16_t >(bytes[offset]) |
                static_cast< std::uint16_t >(static_cast< std::uint16_t >(bytes[offset + 1]) << 8U));
        }
        return image{.width = width, .height = height, .pixels = std::move(pixels)};
    }

    /** Clamps a signed coordinate to an image dimension. */
    auto clamp_coordinate(std::ptrdiff_t coordinate, std::size_t dimension) -> std::size_t
    {
        if (coordinate < 0)
        {
            return 0;
        }
        std::size_t converted = static_cast< std::size_t >(coordinate);
        if (converted >= dimension)
        {
            return dimension - 1;
        }
        return converted;
    }

    /** Applies the deliberately simple scalar reference algorithm. */
    void reference_blur(image& value)
    {
        constexpr std::uint32_t weights[7] = {1, 6, 15, 20, 15, 6, 1};
        std::size_t width = value.width;
        std::size_t height = value.height;
        std::vector< std::uint16_t > temporary(value.pixels.size());

        for (std::size_t iteration = 0; iteration != convolution_iteration_count; ++iteration)
        {
            for (std::size_t row = 0; row != height; ++row)
            {
                for (std::size_t column = 0; column != width; ++column)
                {
                    std::uint32_t sum = 0;
                    for (std::size_t tap = 0; tap != 7; ++tap)
                    {
                        std::ptrdiff_t source_column = static_cast< std::ptrdiff_t >(column) +
                                                       static_cast< std::ptrdiff_t >(tap) - 3;
                        std::size_t clamped_column = clamp_coordinate(source_column, width);
                        sum += weights[tap] * value.pixels[row * width + clamped_column];
                    }
                    temporary[row * width + column] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
                }
            }

            for (std::size_t row = 0; row != height; ++row)
            {
                for (std::size_t column = 0; column != width; ++column)
                {
                    std::uint32_t sum = 0;
                    for (std::size_t tap = 0; tap != 7; ++tap)
                    {
                        std::ptrdiff_t source_row = static_cast< std::ptrdiff_t >(row) +
                                                    static_cast< std::ptrdiff_t >(tap) - 3;
                        std::size_t clamped_row = clamp_coordinate(source_row, height);
                        sum += weights[tap] * temporary[clamped_row * width + column];
                    }
                    value.pixels[row * width + column] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
                }
            }
        }
    }

    /** Checks the candidate image against the independently computed result. */
    auto verify(image const& expected, image const& candidate) -> bool
    {
        if (expected.width != candidate.width || expected.height != candidate.height)
        {
            std::cerr << "dimension mismatch: expected " << expected.width << 'x' << expected.height << ", got "
                      << candidate.width << 'x' << candidate.height << '\n';
            return false;
        }
        for (std::size_t index = 0; index != expected.pixels.size(); ++index)
        {
            if (expected.pixels[index] != candidate.pixels[index])
            {
                std::size_t row = index / expected.width;
                std::size_t column = index % expected.width;
                std::cerr << "pixel mismatch at (" << column << ", " << row << "): expected "
                          << expected.pixels[index] << ", got " << candidate.pixels[index] << '\n';
                return false;
            }
        }
        return true;
    }
}

using process_exit_code = int;
using argument_count = int;

/** Verifies one integer-blur output against its original input. */
auto main(argument_count argc, char** argv) -> process_exit_code
{
    if (argc != 3)
    {
        std::cerr << "usage: verify_integer_blur <input-file> <output-file>\n";
        return 2;
    }

    try
    {
        integer_blur_verifier::image expected = integer_blur_verifier::load_image(argv[1]);
        integer_blur_verifier::image candidate = integer_blur_verifier::load_image(argv[2]);
        integer_blur_verifier::reference_blur(expected);
        if (!integer_blur_verifier::verify(expected, candidate))
        {
            return 1;
        }
    }
    catch (std::exception const& error)
    {
        std::cerr << "verify_integer_blur: " << error.what() << '\n';
        return 1;
    }
    std::cout << "integer blur output is correct\n";
    return 0;
}
