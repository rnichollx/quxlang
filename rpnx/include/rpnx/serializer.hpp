// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef RPNX_SERIALIZER_HPP
#define RPNX_SERIALIZER_HPP

#include <boost/core/demangle.hpp>
#include <concepts>
#include <optional>
#include <vector>
#include <stdint.h>

namespace rpnx
{
    template < typename A, typename... Ts >
    class basic_variant;

    template < typename E >
    struct enum_traits;

    template < typename R, typename V, typename F >
    inline R apply_visitor(F&& func, V&& variant);

    template < typename T, typename It >
    class default_serialization_traits;

    template < typename T, typename It >
    class default_deserialization_traits;

    template < typename T, typename It >
    class json_serialization_traits;

    template < typename T, typename It >
    class cxx_serialization_traits;

    template < typename T >
    class meta_info
    {
      public:
        template < size_t N >
        using nth_member_type = std::remove_reference_t< decltype(std::get< N >(std::declval< T >().tie())) >;

        static constexpr std::size_t member_count = std::tuple_size_v< decltype(std::declval< T >().tie()) >;

        template < size_t N >
        static constexpr std::string nth_member_name()
        {
            return T::strings()[N];
        };

        static std::string type_name()
        {
            return typeid(T).name();
        }
    };

    template < typename T >
    concept has_serial_interface = requires(T t, const T ct) {
        {
            t.serial_interface()
        };
        {
            ct.serial_interface()
        };
    };

    template < typename T >
    concept enum_concept = std::is_enum_v< T >;

    template < typename T, typename OutputIt >
    auto serialize_iter(T const& value, OutputIt output, OutputIt end)
    {
        return default_serialization_traits< T, OutputIt >::serialize_iter(value, output, end);
    }

    template < typename T, typename OutputIt >
    auto serialize_iter(T const& value, OutputIt output)
    {
        return default_serialization_traits< T, OutputIt >::serialize_iter(value, output);
    }

    template < typename T, typename InputIt >
    auto deserialize_iter(T& value, InputIt input, InputIt end) -> InputIt
    {
        return default_deserialization_traits< T, InputIt >::deserialize_iter(value, input, end);
    }

    template < typename T, typename InputIt >
    auto deserialize_iter(T& value, InputIt input) -> InputIt
    {
        return default_deserialization_traits< T, InputIt >::deserialize_iter(value, input);
    }

    template < typename T, typename OutputIt >
    auto json_serialize_iter(T const& value, OutputIt output)
    {
        return json_serialization_traits< T, OutputIt >::serialize_iter(value, output);
    }

    template < typename T, typename OutputIt >
    auto cxx_serialize_iter(T const& value, OutputIt output)
    {
        return cxx_serialization_traits< T, OutputIt >::serialize_iter(value, output);
    }

    namespace detail
    {
        template < typename Tuple, std::size_t... Is, typename It >
        auto serialize_tuple(Tuple const& tuple, std::index_sequence< Is... >, It output, It end) -> It
        {
            ((output = rpnx::serialize_iter(std::get< Is >(tuple), output, end)), ...);
            return output;
        }

        template < typename Tuple, std::size_t... Is, typename It >
        auto serialize_tuple(Tuple const& tuple, std::index_sequence< Is... >, It output) -> It
        {
            ((output = rpnx::serialize_iter(std::get< Is >(tuple), output)), ...);
            return output;
        }

        template < typename Tuple, std::size_t... Is, typename It >
        auto deserialize_tuple(Tuple& tuple, std::index_sequence< Is... >, It input, It end) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(tuple), input, end)), ...);
            return input;
        }

        template < typename Tuple, std::size_t... Is, typename It >
        auto deserialize_tuple(Tuple& tuple, std::index_sequence< Is... >, It input) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(tuple), input)), ...);
            return input;
        }

        template < typename Tuple, std::size_t... Is, typename It >
        auto deserialize_tuple(Tuple&& tuple, std::index_sequence< Is... >, It input, It end) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(std::forward< Tuple >(tuple)), input, end)), ...);
            return input;
        }

        template < typename Tuple, std::size_t... Is, typename It >
        auto deserialize_tuple(Tuple&& tuple, std::index_sequence< Is... >, It input) -> It
        {
            ((input = rpnx::deserialize_iter(std::get< Is >(std::forward< Tuple >(tuple)), input)), ...);
            return input;
        }
    } // namespace detail

    template < typename... Ts, typename It >
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
    };

    template < typename... Ts, typename It >
    class default_deserialization_traits< std::tuple< Ts... >, It >
    {
      public:
        static auto constexpr deserialize_iter(std::tuple< Ts... >& tuple, It input, It end) -> It
        {
            return detail::deserialize_tuple(tuple, std::index_sequence_for< Ts... >{}, input, end);
        }

        static auto constexpr deserialize_iter(std::tuple< Ts... >& tuple, It input) -> It
        {
            return detail::deserialize_tuple(tuple, std::index_sequence_for< Ts... >{}, input);
        }
    };

    template < std::integral I, typename It >
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
    };

    template < std::integral I, typename It >
    class little_endian_deserialization_traits
    {
      public:
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

    template < std::integral I, typename It >
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
    };

    template < std::integral I, typename It >
    class uintany_deserialization_traits
    {
      public:
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

    template < typename M, typename It >
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
    };

    template < typename M, typename It >
    class default_map_deserialization_traits
    {
      public:
        static auto constexpr deserialize_iter(M& output, It input, It end) -> It
        {
            std::size_t size;
            input = uintany_deserialization_traits< std::size_t, It >::deserialize_iter(size, input, end);
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
            input = uintany_deserialization_traits< std::size_t, It >::deserialize_iter(size, input);
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

    template < typename V, typename It >
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
    };

    template < typename V, typename It >
    class default_vector_deserialization_traits
    {
      public:
        static auto constexpr deserialize_iter(V& output, It input, It end) -> It
        {
            std::size_t size;
            input = uintany_deserialization_traits< std::size_t, It >::deserialize_iter(size, input, end);
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
            input = uintany_deserialization_traits< std::size_t, It >::deserialize_iter(size, input);
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

    template < typename S, typename It >
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
    };

    template < typename S, typename It >
    class default_set_deserialization_traits
    {
      public:
        static auto constexpr deserialize_iter(S& output, It input, It end) -> It
        {
            std::size_t size;
            input = uintany_deserialization_traits< std::size_t, It >::deserialize_iter(size, input, end);
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
            input = uintany_deserialization_traits< std::size_t, It >::deserialize_iter(size, input);
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

    template < typename... Ts, typename It >
    auto deserialize_iter(std::tuple< Ts... >&& tuple, It input, It end) -> It
    {
        return detail::deserialize_tuple(std::move(tuple), std::index_sequence_for< Ts... >{}, input, end);
    }

    template < typename... Ts, typename It >
    auto deserialize_iter(std::tuple< Ts... >&& tuple, It input) -> It
    {
        return detail::deserialize_tuple(std::move(tuple), std::index_sequence_for< Ts... >{}, input);
    }

    template < std::integral I, typename It >
    class default_serialization_traits< I, It > : public little_endian_serialization_traits< I, It >
    {
    };

    template < std::integral I, typename It >
    class default_deserialization_traits< I, It > : public little_endian_deserialization_traits< I, It >
    {
    };

    template < typename K, typename V, typename A, typename It >
    class default_serialization_traits< std::map< K, V, A >, It > : public default_map_serialization_traits< std::map< K, V, A >, It >
    {
    };

    template < typename K, typename V, typename A, typename It >
    class default_deserialization_traits< std::map< K, V, A >, It > : public default_map_deserialization_traits< std::map< K, V, A >, It >
    {
    };

    template < typename V, typename A, typename It >
    class default_serialization_traits< std::vector< V, A >, It > : public default_vector_serialization_traits< std::vector< V, A >, It >
    {
    };

    template < typename V, typename A, typename It >
    class default_deserialization_traits< std::vector< V, A >, It > : public default_vector_deserialization_traits< std::vector< V, A >, It >
    {
    };

    template < typename It >
    class default_serialization_traits< std::string, It > : public default_vector_serialization_traits< std::string, It >
    {
    };

    template < typename It >
    class default_deserialization_traits< std::string, It > : public default_vector_deserialization_traits< std::string, It >
    {
    };

    template < typename V, typename A, typename It >
    class default_serialization_traits< std::set< V, A >, It > : public default_set_serialization_traits< std::set< V, A >, It >
    {
    };

    template < typename V, typename A, typename It >
    class default_deserialization_traits< std::set< V, A >, It > : public default_set_deserialization_traits< std::set< V, A >, It >
    {
    };

    template < has_serial_interface S, typename It >
    class default_serialization_traits< S, It >
    {
      public:
        static auto constexpr serialize_iter(S const& input, It output) -> It
        {
            return rpnx::serialize_iter(input.serial_interface(), output);
        }

        static auto constexpr serialize_iter(S const& input, It output, It end) -> It
        {
            return rpnx::serialize_iter(input.serial_interface(), output, end);
        }
    };

    template < has_serial_interface S, typename It >
    class default_deserialization_traits< S, It >
    {
      public:
        static auto constexpr deserialize_iter(S& output, It input) -> It
        {
            return rpnx::deserialize_iter(output.serial_interface(), input);
        }

        static auto constexpr deserialize_iter(S& output, It input, It end) -> It
        {
            return rpnx::deserialize_iter(output.serial_interface(), input, end);
        }
    };

    template < enum_concept E, typename It >
    class default_serialization_traits< E, It >
    {
      public:
        static auto constexpr serialize_iter(E const& input, It output) -> It
        {
            using I = std::underlying_type_t< E >;
            return rpnx::serialize_iter(static_cast< I >(input), output);
        }

        static auto constexpr serialize_iter(E const& input, It output, It end) -> It
        {
            using I = std::underlying_type_t< E >;
            return rpnx::serialize_iter(static_cast< I >(input), output, end);
        }
    };

    template < enum_concept E, typename It >
    class default_deserialization_traits< E, It >
    {
      public:
        static auto constexpr deserialize_iter(E& output, It input) -> It
        {
            using I = std::underlying_type_t< E >;
            I temp{};
            input = rpnx::deserialize_iter(temp, input);
            output = static_cast< E >(temp);
            return input;
        }

        static auto constexpr deserialize_iter(E& output, It input, It end) -> It
        {
            using I = std::underlying_type_t< E >;
            I temp{};
            input = rpnx::deserialize_iter(temp, input, end);
            output = static_cast< E >(temp);
            return input;
        }
    };

    template < typename It >
    class default_serialization_traits< bool, It >
    {
      public:
        // Serialize as 1 byte, 0 or 1
        static auto constexpr serialize_iter(bool const& input, It output) -> It
        {
            return rpnx::serialize_iter(static_cast< std::uint8_t >(input), output);
        }

        static auto constexpr serialize_iter(bool const& input, It output, It end) -> It
        {
            return rpnx::serialize_iter(static_cast< std::uint8_t >(input), output, end);
        }
    };

    template < typename It >
    class default_deserialization_traits< bool, It >
    {
      public:
        // TODO: consider throw for non 1/0 values
        static auto constexpr deserialize_iter(bool& output, It input) -> It
        {
            std::uint8_t temp{};
            input = rpnx::deserialize_iter(temp, input);
            output = temp != 0;
            return input;
        }

        static auto constexpr deserialize_iter(bool& output, It input, It end) -> It
        {
            std::uint8_t temp{};
            input = rpnx::deserialize_iter(temp, input, end);
            output = temp != 0;
            return input;
        }
    };

    template < typename T, typename It >
    class default_serialization_traits< std::optional< T >, It >
    {
      public:
        static auto constexpr serialize_iter(std::optional< T > const& input, It output) -> It
        {
            if (input.has_value())
            {
                *output++ = std::byte(1);
                output = rpnx::serialize_iter(input.value(), output);
            }
            else
            {
                *output++ = std::byte(0);
            }
            return output;
        }

        static auto constexpr serialize_iter(std::optional< T > const& input, It output, It end) -> It
        {
            if (input.has_value())
            {
                *output++ = std::byte(1);
                output = rpnx::serialize_iter(input.value(), output, end);
            }
            else
            {
                *output++ = std::byte(0);
            }
            return output;
        }
    };

    template < typename T, typename It >
    class default_deserialization_traits< std::optional< T >, It >
    {
      public:
        static auto constexpr deserialize_iter(std::optional< T >& output, It input) -> It
        {
            std::byte temp{};
            input = rpnx::deserialize_iter(temp, input);
            if (temp == std::byte(0))
            {
                output = std::nullopt;
            }
            else
            {
                T value{};
                input = rpnx::deserialize_iter(value, input);
                output = std::move(value);
            }
            return input;
        }

        static auto constexpr deserialize_iter(std::optional< T >& output, It input, It end) -> It
        {
            std::byte temp{};
            input = rpnx::deserialize_iter(temp, input, end);
            if (temp == std::byte(0))
            {
                output = std::nullopt;
            }
            else
            {
                T value{};
                input = rpnx::deserialize_iter(value, input, end);
                output = std::move(value);
            }
            return input;
        }
    };

    template < typename T, typename It >
    class cxx_serialization_traits;

    template < typename T, typename It >
    class cxx_serialization_traits< std::optional< T >, It >
    {

      public:
        static auto constexpr serialize_iter(std::optional< T > const& value, It output) -> It
        {
            if (value.has_value())
            {
                output = rpnx::cxx_serialize_iter(value.value(), output);
            }
            else
            {
                std::string null = "std::nullopt";
                for (auto c : null)
                {
                    *output++ = std::byte(c);
                }
            }

            return output;
        }
    };

    template < typename T, typename It >
    class cxx_serialization_traits< std::shared_ptr< T >, It >
    {

      public:
        static auto constexpr serialize_iter(std::shared_ptr< T > const& value, It output) -> It
        {
            if (value != nullptr)
            {
                std::string output_str = "std::make_shared(";
                for (auto c : output_str)
                {
                    *output++ = std::byte(c);
                }

                output = rpnx::cxx_serialize_iter(*value, output);
                *output++ = std::byte(')');
            }
            else
            {
                std::string null = "nullptr";
                for (auto c : null)
                {
                    *output++ = std::byte(c);
                }
            }

            return output;
        }
    };

    template < typename T, typename It >
    class cxx_list_serialization_traits
    {
      public:
        static auto constexpr serialize_iter(T const& value, It output) -> It
        {
            // std::string demangled_typename = typeid(T).name();
            *output++ = std::byte('{');
            bool first = true;
            for (auto it = value.begin(); it != value.end(); ++it)
            {
                if (!first)
                {
                    *output++ = std::byte(',');
                }
                else
                {
                    first = false;
                }
                output = rpnx::cxx_serialize_iter(*it, output);
            }
            *output++ = std::byte('}');
            return output;
        }
    };

    template < has_serial_interface T, typename It >
    class cxx_serialization_traits< T, It >
    {
      private:
        template < std::size_t N >
        static auto constexpr serialize_member(T const& value, It& output)
        {
            if constexpr (N == meta_info< T >::member_count)
            {
                return output;
            }
            else
            {
                if (N != 0)
                {
                    *output++ = std::byte(',');
                    *output++ = std::byte(' ');
                }
                std::string name = meta_info< T >::template nth_member_name< N >();

                *output++ = std::byte('.');
                for (auto c : name)
                {
                    *output++ = std::byte(c);
                }
                *output++ = std::byte(' ');
                *output++ = std::byte('=');
                *output++ = std::byte(' ');
                // output = rpnx::cxx_serialize_iter(name, output);

                auto member = std::get< N >(value.tie());

                output = rpnx::cxx_serialize_iter(member, output);

                return serialize_member< N + 1 >(value, output);
            }
        }

      public:
        static auto constexpr serialize_iter(T const& value, It output) -> It
        {
            std::string typename_str = boost::core::demangle(typeid(T).name());

            for (auto c : typename_str)
            {
                *output++ = std::byte(c);
            }
            *output++ = std::byte('{');
            // std::string type_str = "__TYPE__";
            // output = rpnx::json_serialize_iter(type_str, output);
            //*output++ = std::byte(':');
            // std::string type_name = meta_info< T >::type_name();
            // output = rpnx::json_serialize_iter(type_name, output);

            serialize_member< 0 >(value, output);

            *output++ = std::byte('}');
            return output;
        }
    };

    template < typename It >
    class cxx_serialization_traits< std::string, It >
    {
      public:
        static auto constexpr serialize_iter(std::string const& value, It output) -> It
        {
            *output++ = std::byte('"');
            for (auto c : value)
            {
                if (c == '"')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('"');
                }
                else if (c == '\\')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('\\');
                }
                else if (c == '\n')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('n');
                }
                else if (c == '\r')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('r');
                }
                else if (c == '\t')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('t');
                }
                else if (c == '\b')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('b');
                }
                else if (c == '\f')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('f');
                }
                else
                {
                    *output++ = std::byte(c);
                }
            }
            *output++ = std::byte('"');
            return output;
        }
    };

    template < typename It >
    class cxx_serialization_traits< std::monostate, It >
    {
      public:
        static auto constexpr serialize_iter(std::monostate const& value, It output) -> It
        {
            std::string output_str = "std::monostate{}";
            for (auto c : output_str)
            {
                *output++ = std::byte(c);
            }
            return output;
        }
    };

    template < typename It >
    class cxx_serialization_traits< std::byte, It >
    {
      public:
        static auto constexpr serialize_iter(std::byte const& b, It output) -> It
        {
            std::string output_str = "0x";
            output_str += '0' + (std::to_integer< std::uint8_t >(b) >> 4);
            output_str += '0' + (std::to_integer< std::uint8_t >(b) & 0xF);

            for (auto c : output_str)
            {
                *output++ = std::byte(c);
            }
            return output;
        }
    };

    template < typename T, typename It >
    class json_serialization_traits;

    template < std::integral Int, typename It >
    class json_serialization_traits< Int, It >
    {
      public:
        static auto constexpr serialize_iter(Int const& value, It output) -> It
        {
            std::string int_out;
            int_out = std::to_string(value);
            for (auto c : int_out)
            {
                *output++ = std::byte(c);
            }
            return output;
        }
    };

    template < std::integral Int, typename It >
    class cxx_serialization_traits< Int, It >
    {
      public:
        static auto constexpr serialize_iter(Int const& value, It output) -> It
        {
            std::string int_out;
            int_out = std::to_string(value);
            for (auto c : int_out)
            {
                *output++ = std::byte(c);
            }
            return output;
        }
    };

    template < typename T, typename It >
    class json_list_serialization_traits
    {
      public:
        static auto constexpr serialize_iter(T const& value, It output) -> It
        {
            *output++ = std::byte('[');
            bool first = true;
            for (auto it = value.begin(); it != value.end(); ++it)
            {
                if (!first)
                {
                    *output++ = std::byte(',');
                }
                else
                {
                    first = false;
                }
                output = rpnx::json_serialize_iter(*it, output);
            }
            *output++ = std::byte(']');
            return output;
        }
    };

    template < typename T, typename It >
    class json_serialization_traits< std::optional< T >, It >
    {

      public:
        static auto constexpr serialize_iter(std::optional< T > const& value, It output) -> It
        {
            if (value.has_value())
            {
                output = rpnx::json_serialize_iter(value.value(), output);
            }
            else
            {
                std::string null = "null";
                for (auto c : null)
                {
                    *output++ = std::byte(c);
                }
            }

            return output;
        }
    };

    template < has_serial_interface Struct, typename It >
    class json_serialization_traits< Struct, It >
    {
      private:
        template < std::size_t N >
        static auto constexpr serialize_member(Struct const& value, It& output)
        {
            if constexpr (N == meta_info< Struct >::member_count)
            {
                return output;
            }
            else
            {
                if (N != 0)
                {
                    *output++ = std::byte(',');
                }
                std::string name = meta_info< Struct >::template nth_member_name< N >();

                output = rpnx::json_serialize_iter(name, output);

                auto member = std::get< N >(value.tie());

                *output++ = std::byte(':');
                output = rpnx::json_serialize_iter(member, output);

                return serialize_member< N + 1 >(value, output);
            }
        }

      public:
        static auto constexpr serialize_iter(Struct const& value, It output) -> It
        {
            *output++ = std::byte('{');
            // std::string type_str = "__TYPE__";
            // output = rpnx::json_serialize_iter(type_str, output);
            //*output++ = std::byte(':');
            // std::string type_name = meta_info< T >::type_name();
            // output = rpnx::json_serialize_iter(type_name, output);

            serialize_member< 0 >(value, output);

            *output++ = std::byte('}');
            return output;
        }
    };

    template < typename It >
    class json_serialization_traits< std::string, It >
    {
      public:
        static auto constexpr serialize_iter(std::string const& value, It output) -> It
        {
            *output++ = std::byte('"');
            for (auto c : value)
            {
                if (c == '"')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('"');
                }
                else if (c == '\\')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('\\');
                }
                else if (c == '\n')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('n');
                }
                else if (c == '\r')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('r');
                }
                else if (c == '\t')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('t');
                }
                else if (c == '\b')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('b');
                }
                else if (c == '\f')
                {
                    *output++ = std::byte('\\');
                    *output++ = std::byte('f');
                }
                else
                {
                    *output++ = std::byte(c);
                }
            }
            *output++ = std::byte('"');
            return output;
        }
    };

    // Vector
    template < typename T, typename A, typename It >
    class json_serialization_traits< std::vector< T, A >, It > : public json_list_serialization_traits< std::vector< T, A >, It >
    {
    };

    template < typename T, typename A, typename It >
    class cxx_serialization_traits< std::vector< T, A >, It > : public cxx_list_serialization_traits< std::vector< T, A >, It >
    {
    };

    // Set
    template < typename T, typename C, typename It >
    class json_serialization_traits< std::set< T, C >, It > : public json_list_serialization_traits< std::set< T, C >, It >
    {
    };

    template < typename T, typename C, typename It >
    class cxx_serialization_traits< std::set< T, C >, It > : public cxx_list_serialization_traits< std::set< T, C >, It >
    {
    };

    template < typename A, typename... Ts, typename It >
    class json_serialization_traits< rpnx::basic_variant< A, Ts... >, It >
    {
      public:
        static auto constexpr serialize_iter(rpnx::basic_variant< A, Ts... > const& value, It output) -> It
        {
            *output++ = std::byte('{');
            std::string type_str = "variant_value_type";
            output = rpnx::json_serialize_iter(type_str, output);
            *output++ = std::byte(':');
            std::string type_name = boost::core::demangle(value.type().name());
            // meta_info<rpnx::basic_variant<A, Ts...>>::type_name();
            output = rpnx::json_serialize_iter(type_name, output);

            *output++ = std::byte(',');

            std::string key = "variant_index";
            output = rpnx::json_serialize_iter(key, output);
            *output++ = std::byte(':');
            std::size_t index = value.index();
            output = rpnx::json_serialize_iter(index, output);
            *output++ = std::byte(',');

            std::string value_str = "variant_value";
            output = rpnx::json_serialize_iter(value_str, output);
            *output++ = std::byte(':');
            rpnx::apply_visitor< void >(
                [&](auto const& v)
                {
                    output = rpnx::json_serialize_iter(v, output);
                },
                value);

            *output++ = std::byte('}');
            return output;
        }
    };

    template < typename A, typename... Ts, typename It >
    class cxx_serialization_traits< rpnx::basic_variant< A, Ts... >, It >
    {
      public:
        static auto constexpr serialize_iter(rpnx::basic_variant< A, Ts... > const& value, It output) -> It
        {

            std::string type_name = boost::core::demangle(typeid(rpnx::basic_variant< A, Ts... >).name());
            // meta_info<rpnx::basic_variant<A, Ts...>>::type_name();
            // output = rpnx::cxx_serialize_iter(type_name, output);
            *output++ = std::byte('{');
            rpnx::apply_visitor< void >(
                [&](auto const& v)
                {
                    output = rpnx::cxx_serialize_iter(v, output);
                },
                value);
            *output++ = std::byte('}');
            return output;
        }
    };

    template < enum_concept E, typename It >
    class json_serialization_traits< E, It >
    {
      public:
        static auto constexpr serialize_iter(E const& value, It output) -> It
        {
            auto name = enum_traits< E >::to_string(value);
            return rpnx::json_serialize_iter(name, output);
        }
    };

    template < enum_concept E, typename It >
    class cxx_serialization_traits< E, It >
    {
      public:
        static auto constexpr serialize_iter(E const& value, It output) -> It
        {
            std::string type_name = boost::core::demangle(typeid(E).name());
            auto name = enum_traits< E >::to_string(value);
            type_name += "::";
            type_name += name;
            for (auto c : type_name)
            {
                *output++ = std::byte(c);
            }

            return output;
        }
    };

    template < typename It >
    class json_serialization_traits< bool, It >
    {
      public:
        static auto constexpr serialize_iter(bool const& value, It output) -> It
        {
            if (value)
            {
                *output++ = std::byte('t');
                *output++ = std::byte('r');
                *output++ = std::byte('u');
                *output++ = std::byte('e');
            }
            else
            {
                *output++ = std::byte('f');
                *output++ = std::byte('a');
                *output++ = std::byte('l');
                *output++ = std::byte('s');
                *output++ = std::byte('e');
            }
            return output;
        }
    };

    template < typename It >
    class cxx_serialization_traits< bool, It >
    {
      public:
        static auto constexpr serialize_iter(bool const& value, It output) -> It
        {
            if (value)
            {
                *output++ = std::byte('t');
                *output++ = std::byte('r');
                *output++ = std::byte('u');
                *output++ = std::byte('e');
            }
            else
            {
                *output++ = std::byte('f');
                *output++ = std::byte('a');
                *output++ = std::byte('l');
                *output++ = std::byte('s');
                *output++ = std::byte('e');
            }
            return output;
        }
    };

    template < typename T, typename It >
    class json_serialization_traits< std::map< std::string, T >, It >
    {
      public:
        static auto constexpr serialize_iter(std::map< std::string, T > const& input, It output) -> It
        {
            *output++ = std::byte('{');
            bool first = true;
            for (const auto& [key, value] : input)
            {
                if (!first)
                {
                    *output++ = std::byte(',');
                }
                else
                {
                    first = false;
                }
                output = rpnx::json_serialize_iter(key, output);
                *output++ = std::byte(':');
                output = rpnx::json_serialize_iter(value, output);
            }
            *output++ = std::byte('}');
            return output;
        }
    };

    template < typename T, typename It >
    class cxx_serialization_traits< std::map< std::string, T >, It >
    {
      public:
        static auto constexpr serialize_iter(std::map< std::string, T > const& input, It output) -> It
        {

            *output++ = std::byte('{');
            bool first = true;
            for (const auto& [key, value] : input)
            {
                if (!first)
                {
                    *output++ = std::byte(',');
                }
                else
                {
                    first = false;
                }
                *output++ = std::byte('{');
                output = rpnx::cxx_serialize_iter(key, output);
                *output++ = std::byte(',');
                *output++ = std::byte(' ');
                output = rpnx::cxx_serialize_iter(value, output);
                *output++ = std::byte('}');
            }
            *output++ = std::byte('}');
            return output;
        }
    };
} // namespace rpnx
#endif // RPNX_SERIALIZER_HPP

#ifdef RPNX_VARIANT_HPP
#include "rpnx/compat/variant_serializer.hpp"
#endif