// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef VARIANT_SERIALIZER_HPP
#define VARIANT_SERIALIZER_HPP

#include <array>
#include <rpnx/serialization4.hpp>
#include <rpnx/variant.hpp>

namespace rpnx::serial4
{
    template < typename Allocator, typename... Ts >
    class binary_serial_traits< rpnx::basic_variant< Allocator, Ts... > >
    {
        using variant_type = rpnx::basic_variant< Allocator, Ts... >;

        template < typename T, typename It >
        static auto deserialize_one(variant_type& value, It input) -> It
        {
            T temp{};
            input = rpnx::serial4::deserialize_iter(temp, input);
            value = std::move(temp);
            return input;
        }

        template < typename T, typename It >
        static auto deserialize_one_with_end(variant_type& value, It input, It end) -> It
        {
            T temp{};
            input = rpnx::serial4::deserialize_iter(temp, input, end);
            value = std::move(temp);
            return input;
        }

      public:
        template < typename It >
        static auto constexpr serialize_iter(variant_type const& value, It output, It end) -> It
        {
            output = rpnx::serial4::uintany_serialization_traits< std::size_t, It >::serialize_iter(value.index(), output, end);
            rpnx::apply_visitor< void >(value,
                                        [&](auto const& v)
                                        {
                                            output = rpnx::serial4::serialize_iter(v, output, end);
                                        });
            return output;
        }

        template < typename It >
        static auto constexpr serialize_iter(variant_type const& value, It output) -> It
        {
            output = rpnx::serial4::uintany_serialization_traits< std::size_t, It >::serialize_iter(value.index(), output);
            rpnx::apply_visitor< void >(value,
                                        [&](auto const& v)
                                        {
                                            output = rpnx::serial4::serialize_iter(v, output);
                                        });
            return output;
        }

        template < typename It >
        static auto constexpr deserialize_iter(variant_type& value, It input, It end) -> It
        {
            std::size_t index{};
            input = rpnx::serial4::uintany_deserialization_traits< std::size_t, It >::deserialize_iter(index, input, end);
            if (index >= sizeof...(Ts))
            {
                throw std::runtime_error("Invalid variant index");
            }
            using deserialize_fn = It (*)(variant_type&, It, It);
            static constexpr std::array< deserialize_fn, sizeof...(Ts) > table{&deserialize_one_with_end< Ts, It >...};
            return table[index](value, input, end);
        }

        template < typename It >
        static auto constexpr deserialize_iter(variant_type& value, It input) -> It
        {
            std::size_t index{};
            input = rpnx::serial4::uintany_deserialization_traits< std::size_t, It >::deserialize_iter(index, input);
            if (index >= sizeof...(Ts))
            {
                throw std::runtime_error("Invalid variant index");
            }
            using deserialize_fn = It (*)(variant_type&, It);
            static constexpr std::array< deserialize_fn, sizeof...(Ts) > table{&deserialize_one< Ts, It >...};
            return table[index](value, input);
        }
    };

} // namespace rpnx::serial4

#endif // VARIANT_SERIALIZER_HPP
