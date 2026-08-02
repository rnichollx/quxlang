#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <vector>

namespace integer_blur
{
    using file_descriptor = int;

    constexpr std::size_t file_header_size = 16;
    constexpr std::size_t convolution_radius = 3;
    constexpr std::size_t convolution_iteration_count = 12;
    constexpr std::uint32_t maximum_dimension = 65'536;
    constexpr std::uint8_t file_magic[8] = {'Q', 'I', 'M', 'G', '1', '6', 'L', 'E'};

    struct image
    {
        std::uint32_t width;
        std::uint32_t height;
        std::vector< std::uint16_t > pixels;
    };

    /** Closes one POSIX file descriptor when control leaves its scope. */
    class owned_file
    {
    public:
        /** Takes ownership of descriptor. */
        explicit owned_file(file_descriptor descriptor) : descriptor_(descriptor) {}

        owned_file(owned_file const&) = delete;
        auto operator=(owned_file const&) -> owned_file& = delete;

        /** Closes the owned descriptor. */
        ~owned_file()
        {
            if (descriptor_ >= 0)
            {
                ::close(descriptor_);
            }
        }

        /** Returns the owned descriptor. */
        auto get() -> file_descriptor
        {
            return descriptor_;
        }

        /** Closes the descriptor and reports whether the close succeeded. */
        auto close() -> bool
        {
            file_descriptor descriptor = descriptor_;
            descriptor_ = -1;
            return ::close(descriptor) == 0;
        }

    private:
        file_descriptor descriptor_;
    };

    /** Decodes one little-endian 32-bit integer. */
    auto decode_u32(std::uint8_t const* bytes) -> std::uint32_t
    {
        return static_cast< std::uint32_t >(bytes[0]) |
               (static_cast< std::uint32_t >(bytes[1]) << 8U) |
               (static_cast< std::uint32_t >(bytes[2]) << 16U) |
               (static_cast< std::uint32_t >(bytes[3]) << 24U);
    }

    /** Encodes one 32-bit integer in little-endian byte order. */
    void encode_u32(std::uint8_t* bytes, std::uint32_t value)
    {
        bytes[0] = static_cast< std::uint8_t >(value);
        bytes[1] = static_cast< std::uint8_t >(value >> 8U);
        bytes[2] = static_cast< std::uint8_t >(value >> 16U);
        bytes[3] = static_cast< std::uint8_t >(value >> 24U);
    }

    /** Reads exactly byte_count bytes or rejects an early end of file. */
    void read_complete(file_descriptor descriptor, std::uint8_t* bytes, std::size_t byte_count)
    {
        std::size_t offset = 0;
        while (offset != byte_count)
        {
            ssize_t count = ::read(descriptor, bytes + offset, byte_count - offset);
            if (count < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                throw std::runtime_error("input read failed");
            }
            if (count == 0)
            {
                throw std::runtime_error("input is truncated");
            }
            offset += static_cast< std::size_t >(count);
        }
    }

    /** Writes exactly byte_count bytes. */
    void write_complete(file_descriptor descriptor, std::uint8_t const* bytes, std::size_t byte_count)
    {
        std::size_t offset = 0;
        while (offset != byte_count)
        {
            ssize_t count = ::write(descriptor, bytes + offset, byte_count - offset);
            if (count < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                throw std::runtime_error("output write failed");
            }
            if (count == 0)
            {
                throw std::runtime_error("output write made no progress");
            }
            offset += static_cast< std::size_t >(count);
        }
    }

    /** Reads and validates one QIMG16LE image. */
    auto read_image(std::string_view path) -> image
    {
        std::string path_text(path);
        owned_file input(::open(path_text.c_str(), O_RDONLY));
        if (input.get() < 0)
        {
            throw std::runtime_error("could not open input");
        }

        std::uint8_t header[file_header_size];
        read_complete(input.get(), header, file_header_size);
        for (std::size_t index = 0; index != 8; ++index)
        {
            if (header[index] != file_magic[index])
            {
                throw std::runtime_error("input has an invalid QIMG16LE header");
            }
        }

        std::uint32_t width = decode_u32(header + 8);
        std::uint32_t height = decode_u32(header + 12);
        if (width < 7 || height < 7 || width > maximum_dimension || height > maximum_dimension)
        {
            throw std::runtime_error("input dimensions are outside the supported range");
        }

        std::size_t pixel_count = static_cast< std::size_t >(width) * static_cast< std::size_t >(height);
        if (pixel_count > (std::numeric_limits< std::size_t >::max() - file_header_size) / 2)
        {
            throw std::runtime_error("input dimensions overflow the file size");
        }

        std::vector< std::uint8_t > encoded_pixels(pixel_count * 2);
        read_complete(input.get(), encoded_pixels.data(), encoded_pixels.size());
        std::uint8_t trailing_byte = 0;
        ssize_t trailing_count = ::read(input.get(), &trailing_byte, 1);
        if (trailing_count != 0)
        {
            throw std::runtime_error("input contains trailing bytes or could not be checked");
        }
        if (!input.close())
        {
            throw std::runtime_error("could not close input");
        }

        std::vector< std::uint16_t > pixels(pixel_count);
        for (std::size_t index = 0; index != pixel_count; ++index)
        {
            std::size_t byte_offset = index * 2;
            pixels[index] = static_cast< std::uint16_t >(
                static_cast< std::uint16_t >(encoded_pixels[byte_offset]) |
                static_cast< std::uint16_t >(static_cast< std::uint16_t >(encoded_pixels[byte_offset + 1]) << 8U));
        }
        return image{.width = width, .height = height, .pixels = std::move(pixels)};
    }

    /** Writes one image using the QIMG16LE format. */
    void write_image(std::string_view path, image const& value)
    {
        std::string path_text(path);
        owned_file output(::open(path_text.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
        if (output.get() < 0)
        {
            throw std::runtime_error("could not open output");
        }

        std::vector< std::uint8_t > bytes(file_header_size + value.pixels.size() * 2);
        for (std::size_t index = 0; index != 8; ++index)
        {
            bytes[index] = file_magic[index];
        }
        encode_u32(bytes.data() + 8, value.width);
        encode_u32(bytes.data() + 12, value.height);
        for (std::size_t index = 0; index != value.pixels.size(); ++index)
        {
            std::uint16_t pixel = value.pixels[index];
            bytes[file_header_size + index * 2] = static_cast< std::uint8_t >(pixel);
            bytes[file_header_size + index * 2 + 1] = static_cast< std::uint8_t >(pixel >> 8U);
        }
        write_complete(output.get(), bytes.data(), bytes.size());
        if (!output.close())
        {
            throw std::runtime_error("could not close output");
        }
    }

    /** Returns one horizontally clamped source pixel. */
    auto horizontal_pixel(
        std::vector< std::uint16_t > const& source,
        std::size_t width,
        std::size_t row,
        std::ptrdiff_t column) -> std::uint16_t
    {
        std::ptrdiff_t maximum_column = static_cast< std::ptrdiff_t >(width - 1);
        std::ptrdiff_t clamped_column = column;
        if (clamped_column < 0)
        {
            clamped_column = 0;
        }
        if (clamped_column > maximum_column)
        {
            clamped_column = maximum_column;
        }
        return source[row * width + static_cast< std::size_t >(clamped_column)];
    }

    /** Applies the horizontal 7-tap pass. */
    void horizontal_pass(
        std::vector< std::uint16_t > const& source,
        std::vector< std::uint16_t >& destination,
        std::size_t width,
        std::size_t height)
    {
        for (std::size_t row = 0; row != height; ++row)
        {
            std::size_t row_offset = row * width;
            for (std::size_t column = 0; column != convolution_radius; ++column)
            {
                std::ptrdiff_t signed_column = static_cast< std::ptrdiff_t >(column);
                std::uint32_t sum = horizontal_pixel(source, width, row, signed_column - 3) +
                                    6U * horizontal_pixel(source, width, row, signed_column - 2) +
                                    15U * horizontal_pixel(source, width, row, signed_column - 1) +
                                    20U * horizontal_pixel(source, width, row, signed_column) +
                                    15U * horizontal_pixel(source, width, row, signed_column + 1) +
                                    6U * horizontal_pixel(source, width, row, signed_column + 2) +
                                    horizontal_pixel(source, width, row, signed_column + 3);
                destination[row_offset + column] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
            }

            for (std::size_t column = convolution_radius; column != width - convolution_radius; ++column)
            {
                std::size_t index = row_offset + column;
                std::uint32_t sum = source[index - 3] + 6U * source[index - 2] + 15U * source[index - 1] +
                                    20U * source[index] + 15U * source[index + 1] + 6U * source[index + 2] +
                                    source[index + 3];
                destination[index] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
            }

            for (std::size_t column = width - convolution_radius; column != width; ++column)
            {
                std::ptrdiff_t signed_column = static_cast< std::ptrdiff_t >(column);
                std::uint32_t sum = horizontal_pixel(source, width, row, signed_column - 3) +
                                    6U * horizontal_pixel(source, width, row, signed_column - 2) +
                                    15U * horizontal_pixel(source, width, row, signed_column - 1) +
                                    20U * horizontal_pixel(source, width, row, signed_column) +
                                    15U * horizontal_pixel(source, width, row, signed_column + 1) +
                                    6U * horizontal_pixel(source, width, row, signed_column + 2) +
                                    horizontal_pixel(source, width, row, signed_column + 3);
                destination[row_offset + column] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
            }
        }
    }

    /** Returns one vertically clamped source pixel. */
    auto vertical_pixel(
        std::vector< std::uint16_t > const& source,
        std::size_t width,
        std::size_t height,
        std::ptrdiff_t row,
        std::size_t column) -> std::uint16_t
    {
        std::ptrdiff_t maximum_row = static_cast< std::ptrdiff_t >(height - 1);
        std::ptrdiff_t clamped_row = row;
        if (clamped_row < 0)
        {
            clamped_row = 0;
        }
        if (clamped_row > maximum_row)
        {
            clamped_row = maximum_row;
        }
        return source[static_cast< std::size_t >(clamped_row) * width + column];
    }

    /** Applies the vertical 7-tap pass. */
    void vertical_pass(
        std::vector< std::uint16_t > const& source,
        std::vector< std::uint16_t >& destination,
        std::size_t width,
        std::size_t height)
    {
        for (std::size_t row = 0; row != convolution_radius; ++row)
        {
            std::ptrdiff_t signed_row = static_cast< std::ptrdiff_t >(row);
            for (std::size_t column = 0; column != width; ++column)
            {
                std::uint32_t sum = vertical_pixel(source, width, height, signed_row - 3, column) +
                                    6U * vertical_pixel(source, width, height, signed_row - 2, column) +
                                    15U * vertical_pixel(source, width, height, signed_row - 1, column) +
                                    20U * vertical_pixel(source, width, height, signed_row, column) +
                                    15U * vertical_pixel(source, width, height, signed_row + 1, column) +
                                    6U * vertical_pixel(source, width, height, signed_row + 2, column) +
                                    vertical_pixel(source, width, height, signed_row + 3, column);
                destination[row * width + column] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
            }
        }

        for (std::size_t row = convolution_radius; row != height - convolution_radius; ++row)
        {
            std::size_t row_offset = row * width;
            for (std::size_t column = 0; column != width; ++column)
            {
                std::size_t index = row_offset + column;
                std::uint32_t sum = source[index - 3 * width] + 6U * source[index - 2 * width] +
                                    15U * source[index - width] + 20U * source[index] +
                                    15U * source[index + width] + 6U * source[index + 2 * width] +
                                    source[index + 3 * width];
                destination[index] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
            }
        }

        for (std::size_t row = height - convolution_radius; row != height; ++row)
        {
            std::ptrdiff_t signed_row = static_cast< std::ptrdiff_t >(row);
            for (std::size_t column = 0; column != width; ++column)
            {
                std::uint32_t sum = vertical_pixel(source, width, height, signed_row - 3, column) +
                                    6U * vertical_pixel(source, width, height, signed_row - 2, column) +
                                    15U * vertical_pixel(source, width, height, signed_row - 1, column) +
                                    20U * vertical_pixel(source, width, height, signed_row, column) +
                                    15U * vertical_pixel(source, width, height, signed_row + 1, column) +
                                    6U * vertical_pixel(source, width, height, signed_row + 2, column) +
                                    vertical_pixel(source, width, height, signed_row + 3, column);
                destination[row * width + column] = static_cast< std::uint16_t >((sum + 32U) >> 6U);
            }
        }
    }

    /** Executes the complete fixed benchmark workload. */
    void blur(image& value)
    {
        std::size_t width = value.width;
        std::size_t height = value.height;
        std::vector< std::uint16_t > temporary(value.pixels.size());
        for (std::size_t iteration = 0; iteration != convolution_iteration_count; ++iteration)
        {
            horizontal_pass(value.pixels, temporary, width, height);
            vertical_pass(temporary, value.pixels, width, height);
        }
    }
}

using process_exit_code = int;
using argument_count = int;

/** Runs the portable C++ integer-blur benchmark. */
auto main(argument_count argc, char** argv) -> process_exit_code
{
    if (argc == 2 && std::string_view(argv[1]) == "--report-stepping")
    {
        std::cout << "compiler-default\n";
        return 0;
    }
    if (argc != 3)
    {
        std::cerr << "usage: integer_blur <input-file> <output-file>\n";
        return 2;
    }

    try
    {
        integer_blur::image value = integer_blur::read_image(argv[1]);
        integer_blur::blur(value);
        integer_blur::write_image(argv[2], value);
    }
    catch (std::exception const& error)
    {
        std::cerr << "integer_blur: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
