// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef RPNX_SERIALIZER_HPP
#define RPNX_SERIALIZER_HPP

#include <concepts>
#include <optional>

namespace rpnx
{

    template <typename T, typename It>
    class default_serialization_traits;

    template <typename T, typename OutputIt>
    auto serialize_iter(T const& value, OutputIt output, OutputIt end)
    {
        return default_serialization_traits< T, OutputIt >::serialize_iter(value, output, end);
    }

    template <typename T, typename OutputIt>
    auto serialize_iter(T const& value, OutputIt output)
    {
        return default_serialization_traits< T, OutputIt >::serialize_iter(value, output);
    }

    template <typename T, typename InputIt>
    auto deserialize_iter(T& value, InputIt input, InputIt end)
    {
        return default_serialization_traits< T, InputIt >::deserialize_iter(value, input, end);
    }

    template <typename T, typename InputIt>
    auto deserialize_iter(T& value, InputIt input)
    {
        return default_serialization_traits< T, InputIt >::deserialize_iter(value, input);
    }


    namespace detail
    {
        template <typename Tuple, std::size_t... Is, typename It>
        auto serialize_tuple(Tuple const& tuple, std::index_sequence< Is... >, It output, It end) -> It
        {
            ((output = rpnx::serialize_iter(std::get< Is >(tuple), output, end)), ...);
            return output;
        }

        template <typename Tuple, std::size_t... Is, typename It>
        auto serialize_tuple(Tuple const& tuple, std::index_sequence< Is... >, It output) -> It
        {
            ((output = rpnx::serialize_iter(std::get< Is >(tuple), output)), ...);
            return output;
        }

        template <typename Tuple, std::size_t... Is, typename It>
        auto deserialize_tuple(Tuple& tuple, std::index_sequence< Is... >, It input, It end) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(tuple), input, end)), ...);
            return input;
        }

        template <typename Tuple, std::size_t... Is, typename It>
        auto deserialize_tuple(Tuple& tuple, std::index_sequence< Is... >, It input) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(tuple), input)), ...);
            return input;
        }

        template <typename Tuple, std::size_t... Is, typename It>
        auto deserialize_tuple(Tuple&& tuple, std::index_sequence< Is... >, It input, It end) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(std::forward< Tuple >(tuple)), input, end)), ...);
            return input;
        }

        template <typename Tuple, std::size_t... Is, typename It>
        auto deserialize_tuple(Tuple&& tuple, std::index_sequence< Is... >, It input) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(std::forward< Tuple >(tuple)), input)), ...);
            return input;
        }
    }

    template <typename... Ts, typename It>
    class default_serialization_traits< std::tuple< Ts... >, It >
    {
    public:
        static auto constexpr serialize_iter(std::tuple< Ts... > const& tuple, It output, It end) -> It
        {
            return detail::serialize_tuple(tuple, std::index_sequence_for< Ts... >{}, output, end);
        }

        static auto constexpr serialize_iter(std::tuple< Ts... > const& tuple, It output) -> It
        {
            return detail::serialize_tuple(tuple, std::index_sequence_for< Ts... >{}, output);
        }

        static auto constexpr deserialize_iter(std::tuple< Ts... >& tuple, It input, It end) -> It
        {
            return detail::deserialize_tuple(tuple, std::index_sequence_for< Ts... >{}, input, end);
        }

        static auto constexpr deserialize_iter(std::tuple< Ts... >& tuple, It input) -> It
        {
            return detail::deserialize_tuple(tuple, std::index_sequence_for< Ts... >{}, input);
        }
    };


    template <std::integral I, typename It>
    class little_endian_serialization_traits
    {
    public:
        static constexpr It serialize_iter(I const& input, It begin, It end)
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

            return begin;
        }

        static constexpr It serialize_iter(I const& input, It begin)
        {
            for (int i = 0; i < sizeof(I); i++)
            {
                *begin = std::byte((input >> (i * 8)) & I(0xFF));
                begin++;
            }

            return begin;
        }

        static constexpr It deserialize_iter(I& output, It begin, It end)
        {
            output = 0;
            for (int i = 0; i < sizeof(I); i++)
            {
                if (begin == end)
                {
                    throw std::range_error("unexpected end of input range");
                }
                output |= static_cast< I >(static_cast< unsigned char >(*begin)) << (i * 8);
                begin++;
            }

            return begin;
        }

        static constexpr It deserialize_iter(I& output, It begin)
        {
            output = 0;
            for (int i = 0; i < sizeof(I); i++)
            {
                output |= static_cast< I >(static_cast< std::uint8_t >(*begin)) << (i * 8);
                begin++;
            }

            return begin;
        }
    };


    template <std::integral I, typename It>
    class uintany_serialization_traits
    {
    public:
        static constexpr auto serialize_iter(I input, It out) -> It
        {
            uintmax_t base = input;
            uintmax_t bytecount = 1;
            uintmax_t max = 0;
            max = (1ull << (7)) - 1;
            while (base > max)
            {
                bytecount++;
                base -= max + 1;
                max = (1ull << ((7) * bytecount)) - 1;
            }
            for (uintmax_t i = 0; i < bytecount; i++)
            {
                uint8_t val = base & ((uintmax_t(1) << (7)) - 1);
                if (i != bytecount - 1)
                {
                    val |= 0b10000000;
                }
                *out++ = std::byte(val);
                base >>= 7;
            }
            return out;
        }

        static constexpr auto serialize_iter(I input, It out, It end) -> It
        {
            uintmax_t base = input;
            uintmax_t bytecount = 1;
            uintmax_t max = 0;
            max = (1ull << (7)) - 1;
            while (base > max)
            {
                bytecount++;
                base -= max + 1;
                max = (1ull << ((7) * bytecount)) - 1;
            }
            for (uintmax_t i = 0; i < bytecount; i++)
            {
                if (out == end)
                {
                    throw std::range_error("unexpected end of output range");
                }
                uint8_t val = base & ((uintmax_t(1) << (7)) - 1);
                if (i != bytecount - 1)
                {
                    val |= 0b10000000;
                }
                *out++ = std::byte(val);
                base >>= 7;
            }
            return out;
        }

        static constexpr auto deserialize_iter(I& output, It input) -> It
        {
            output = 0;
            uintmax_t n2 = 0;
            while (true)
            {
                std::uint8_t a = static_cast< std::uint8_t >(*input++);
                std::uint8_t read_value = a & 0b1111111;
                output += (uintmax_t(read_value) << (n2 * 7));
                if (!(a & 0b10000000))
                    break;
                n2++;
            }
            for (uintmax_t i = 1; i <= n2; i++)
            {
                output += (uintmax_t(1) << (i * 7));
            }
            return input;
        }

        static constexpr auto deserialize_iter(I& output, It input, It input_end) -> It
        {
            output = 0;
            uintmax_t n2 = 0;
            while (true)
            {
                if (input == input_end)
                {
                    throw std::out_of_range("deserialization input range error");
                }
                std::uint8_t a = static_cast< std::uint8_t >(*input++);
                std::uint8_t read_value = a & 0b1111111;
                output += (uintmax_t(read_value) << (n2 * 7));
                if (!(a & 0b10000000))
                {
                    break;
                }
                n2++;
            }
            for (uintmax_t i = 1; i <= n2; i++)
            {
                output += (uintmax_t(1) << (i * 7));
            }
            return input;
        }


    };


    template <typename M, typename It>
    class default_map_serialization_traits
    {
    public:
        static auto constexpr serialize_iter(M const& input, It output, It end) -> It
        {
            output = uintany_serialization_traits< std::size_t, It >::serialize_iter(input.size(), output, end);
            for (auto& [key, value] : input)
            {
                output = serialize(key, output, end);
                output = serialize(value, output, end);
            }
            return output;
        }

        static auto constexpr serialize_iter(M const& input, It output) -> It
        {
            output = uintany_serialization_traits< std::size_t, It >::serialize_iter(input.size(), output);
            for (auto& pair : input)
            {
                auto const& key = pair.first;
                auto const& value = pair.second;
                output = rpnx::serialize_iter(key, output);
                output = rpnx::serialize_iter(value, output);
            }
            return output;
        }

        static auto constexpr deserialize_iter(M& output, It input, It end) -> It
        {
            std::size_t size;
            input = uintany_serialization_traits< std::size_t, It >::deserialize_iter(size, input, end);
            output.clear();
            for (std::size_t i = 0; i < size; ++i)
            {
                typename M::key_type key;
                typename M::mapped_type value;
                input = rpnx::deserialize_iter(key, input, end);
                input = rpnx::deserialize_iter(value, input, end);
                output.emplace(std::move(key), std::move(value));
            }
            return input;
        }

        static auto constexpr deserialize_iter(M& output, It input) -> It
        {
            std::size_t size;
            input = uintany_serialization_traits< std::size_t, It >::deserialize_iter(size, input);
            output.clear();
            for (std::size_t i = 0; i < size; ++i)
            {
                typename M::key_type key;
                typename M::mapped_type value;
                input = rpnx::deserialize_iter(key, input);
                input = rpnx::deserialize_iter(value, input);
                output.emplace(std::move(key), std::move(value));
            }
            return input;
        }
    };

    template <typename V, typename It>
    class default_vector_serialization_traits
    {
    public:
        static auto constexpr serialize_iter(V const& input, It output, It end) -> It
        {
            output = uintany_serialization_traits< std::size_t, It >::serialize_iter(input.size(), output, end);
            for (auto const& value : input)
            {
                output = rpnx::serialize_iter(value, output, end);
            }
            return output;
        }

        static auto constexpr serialize_iter(V const& input, It output) -> It
        {
            output = uintany_serialization_traits< std::size_t, It >::serialize_iter(input.size(), output);
            for (auto const& value : input)
            {
                output = rpnx::serialize_iter(value, output);
            }
            return output;
        }

        static auto constexpr deserialize_iter(V& output, It input, It end) -> It
        {
            std::size_t size;
            input = uintany_serialization_traits< std::size_t, It >::deserialize_iter(size, input, end);
            output.clear();
            output.reserve(size);
            for (std::size_t i = 0; i < size; ++i)
            {
                typename V::value_type value;
                input = rpnx::deserialize_iter(value, input, end);
                output.push_back(std::move(value));
            }
            return input;
        }

        static auto constexpr deserialize_iter(V& output, It input) -> It
        {
            std::size_t size;
            input = uintany_serialization_traits< std::size_t, It >::deserialize_iter(size, input);
            output.clear();
            output.reserve(size);
            for (std::size_t i = 0; i < size; ++i)
            {
                typename V::value_type value;
                input = rpnx::deserialize_iter(value, input);
                output.push_back(std::move(value));
            }
            return input;
        }

    };

    template <typename S, typename It>
    class default_set_serialization_traits
    {
    public:
        static auto constexpr serialize_iter(S const& input, It output, It end) -> It
        {
            output = uintany_serialization_traits< std::size_t, It >::serialize_iter(input.size(), output, end);
            for (auto const& value : input)
            {
                output = rpnx::serialize_iter(value, output, end);
            }
            return output;
        }

        static auto constexpr serialize_iter(S const& input, It output) -> It
        {
            output = uintany_serialization_traits< std::size_t, It >::serialize_iter(input.size(), output);
            for (auto const& value : input)
            {
                output = rpnx::serialize_iter(value, output);
            }
            return output;
        }

        static auto constexpr deserialize_iter(S& output, It input, It end) -> It
        {
            std::size_t size;
            input = uintany_serialization_traits< std::size_t, It >::deserialize_iter(size, input, end);
            output.clear();
            for (std::size_t i = 0; i < size; ++i)
            {
                typename S::value_type value;
                input = rpnx::deserialize_iter(value, input, end);
                output.insert(std::move(value));
            }
            return input;
        }

        static auto constexpr deserialize_iter(S& output, It input) -> It
        {
            std::size_t size;
            input = uintany_serialization_traits< std::size_t, It >::deserialize_iter(size, input);
            output.clear();
            for (std::size_t i = 0; i < size; ++i)
            {
                typename S::value_type value;
                input = rpnx::deserialize_iter(value, input);
                output.insert(std::move(value));
            }
            return input;
        }
    };

    template <typename... Ts, typename It>
    auto deserialize_iter(std::tuple< Ts... >&& tuple, It input, It end) -> It
    {
        return detail::deserialize_tuple(std::move(tuple), std::index_sequence_for< Ts... >{}, input, end);
    }

    template <typename... Ts, typename It>
    auto deserialize_iter(std::tuple< Ts... >&& tuple, It input) -> It
    {
        return detail::deserialize_tuple(std::move(tuple), std::index_sequence_for< Ts... >{}, input);
    }


    template <std::integral I, typename It>
    class default_serialization_traits< I, It >
        : public little_endian_serialization_traits< I, It >
    {
    };

    template <typename K, typename V, typename A, typename It>
    class default_serialization_traits< std::map< K, V, A >, It >
        : public default_map_serialization_traits< std::map< K, V, A >, It >
    {
    };

    template <typename V, typename A, typename It>
    class default_serialization_traits< std::vector< V, A >, It >
        : public default_vector_serialization_traits< std::vector< V, A >, It >
    {
    };

    template <typename It>
    class default_serialization_traits< std::string, It >
        : public default_vector_serialization_traits< std::string, It >
    {
    };

    template <typename V, typename A, typename It>
    class default_serialization_traits< std::set< V, A >, It >
        : public default_set_serialization_traits< std::set< V, A >, It >
    {
    };

}
#endif //SERIALIZER_HPP


#ifdef RPNX_VARIANT_HPP
#include "rpnx/compat/variant_serializer.hpp"
#endif