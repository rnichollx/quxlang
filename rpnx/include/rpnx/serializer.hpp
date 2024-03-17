// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <concepts>
#include <optional>

namespace rpnx
{


    inline void write_byte(std::byte &out, std::byte in)
    {
        out = in;
    }

    inline void write_byte(std::uint8_t &out, std::byte in)
    {
        out = static_cast<std::uint8_t>(in);
    }

    inline void write_byte(std::int8_t &out, std::byte in)
    {
        out = static_cast<std::int8_t>(in);
    }

    inline void write_byte(signed char &out, std::byte in)
    {
        out = static_cast<char>(in);
    }

    inline void write_byte(unsigned char &out, std::byte in)
    {
        out = static_cast<unsigned char>(in);
    }

    template <std::integral I, typename It>
    class little_endian_serialization_traits
    {
    public:
        It serialize_iter(I const & input, It begin, It end)
        {
            for (int i = 0; i < sizeof(I); i++)
            {
                if (begin == end)
                {
                    throw std::range_error("unexpected end of output range");
                }
                write_byte(*begin, std::byte((input >> (i * 8)) & I(0xFF)));
                begin++;
            }
        }

        It serialize_iter(I const & input, It begin)
        {
            for (int i = 0; i < sizeof(I); i++)
            {
                write_byte(*begin, std::byte((input >> (i * 8)) & I(0xFF)));
                begin++;
            }
        }
    };


    template <std::integral I, typename It>
    class uintany_serialization_traits
    {
    };


    template <typename M, typename It>
    class default_map_serialization_traits
    {
    public:

    };

    template <typename T, typename It>
    class default_serialization_traits;


    template <std::integral I, typename It>
    class default_serialization_traits< I, It >
        : public little_endian_serialization_traits<It, I>
    {
    };









}
#endif //SERIALIZER_HPP