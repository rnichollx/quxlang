#include <cstdint>
#ifndef QUXLANG_BENCHMARKSGAME_CPP_OUTPUT_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_OUTPUT_HPP
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string_view>

/** Buffers standard output in fixed-size blocks. */
class output_buffer
{
    std::array< std::uint8_t, 16384 > bytes_{};
    std::size_t used_ = 0;

  public:
    /** Writes all pending bytes or terminates on output failure. */
    void flush()
    {
        if (std::fwrite(bytes_.data(), 1, used_, stdout) != used_)
        {
            std::abort();
        }
        used_ = 0;
    }

    /** Appends one byte. */
    void write_byte(std::uint8_t value)
    {
        bytes_[used_++] = value;
        if (used_ == bytes_.size())
        {
            flush();
        }
    }

    /** Appends the bytes of a string view. */
    void write_text(std::string_view text)
    {
        for (std::uint8_t value : text)
        {
            write_byte(value);
        }
    }
};

#endif
