// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef VARIANT_SERIALIZER_HPP
#define VARIANT_SERIALIZER_HPP

namespace rpnx
{
    template <typename Allocator, typename... Ts, typename It>
    class default_serialization_traits< rpnx::basic_variant< Allocator, Ts... >, It >
    {
      public:
        static auto constexpr serialize_iter(rpnx::basic_variant< Allocator, Ts... > const& value, It output, It end) -> It
        {
            output = rpnx::uintany_serialization_traits< std::size_t, It >::serialize_iter(value.index(), output, end);
            rpnx::apply_visitor< It >([&output, end](auto const& v)
                                      {
                                          output = rpnx::serialize_iter(v, output, end);
                                          return output;
                                      }, value);
            return output;
        }

        static auto constexpr serialize_iter(rpnx::basic_variant< Allocator, Ts... > const& value, It output) -> It
        {
            output = rpnx::uintany_serialization_traits< std::size_t, It >::serialize_iter(value.index(), output);
            rpnx::apply_visitor< It >([&output](auto const& v)
                                      {
                                          output = rpnx::serialize_iter(v, output);
                                          return output;
                                      }, value);
            return output;
        }
    };

    template <typename Allocator, typename... Ts, typename It>
    class default_deserialization_traits< rpnx::basic_variant< Allocator, Ts... >, It >
    {
      private:
        struct vtable
        {
            void (*deserialize1)(rpnx::basic_variant< Allocator, Ts... >&, It&);
            void (*deserialize2)(rpnx::basic_variant< Allocator, Ts... >&, It&, It);
        };

        template <typename T>
        static constexpr void deserialize1_fn(rpnx::basic_variant< Allocator, Ts... >& value, It& input)
        {
            T temp;
            input = rpnx::deserialize_iter(temp, input);
            value = std::move(temp);
        }

        template <typename T>
        static constexpr void deserialize2_fn(rpnx::basic_variant< Allocator, Ts... >& value, It& input, It end)
        {
            T temp;
            input = rpnx::deserialize_iter(temp, input, end);
            value = std::move(temp);
        }

        template <typename T>
        static constexpr inline vtable vtable_for_index = vtable{
            .deserialize1 = &deserialize1_fn< T >,
            .deserialize2 = &deserialize2_fn< T >
        };

        static constexpr vtable const* vtable_for_deserialize[sizeof...(Ts)] = {
            &vtable_for_index< Ts >...
        };

      public:
        static auto constexpr deserialize_iter(rpnx::basic_variant< Allocator, Ts... >& value, It input, It end) -> It
        {
            std::size_t index{};
            input = rpnx::uintany_deserialization_traits< std::size_t, It >::deserialize_iter(index, input, end);
            if (index >= sizeof...(Ts))
            {
                throw std::runtime_error("Invalid variant index");
            }
            vtable_for_deserialize[index]->deserialize2(value, input, end);
            return input;
        }

        static auto constexpr deserialize_iter(rpnx::basic_variant< Allocator, Ts... >& value, It input) -> It
        {
            std::size_t index{};
            input = rpnx::uintany_deserialization_traits< std::size_t, It >::deserialize_iter(index, input);
            if (index >= sizeof...(Ts))
            {
                throw std::runtime_error("Invalid variant index");
            }
            vtable_for_deserialize[index]->deserialize1(value, input);
            return input;
        }
    };
}

#endif //VARIANT_SERIALIZER_HPP