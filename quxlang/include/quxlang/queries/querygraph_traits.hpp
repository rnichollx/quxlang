// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_QUERYGRAPH_TRAITS_HEADER_GUARD
#define QUXLANG_QUERIES_QUERYGRAPH_TRAITS_HEADER_GUARD

#include <quxlang/data/machine.hpp>
#include <quxlang/data/target_configuration.hpp>

#include <rpnx/compat/variant_serializer.hpp>
#include <rpnx/querygraph/querygraph.hpp>
#include <rpnx/serialization4.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <variant>
#include <vector>

namespace quxlang
{
    namespace detail
    {
        template < typename H >
        constexpr void hash_type_string(H& h, std::string_view value)
        {
            std::size_t c = 0;
            std::uint64_t z = 0;

            for (char const ch : value)
            {
                if (c++ == 8)
                {
                    h(z);
                    c = 0;
                    z = 0;
                }

                z <<= 8;
                z |= static_cast< std::uint8_t >(ch);
            }

            if (c > 0)
            {
                h(z);
            }
        }

        template < typename H >
        constexpr void hash_struct_prefix(H& h, std::size_t member_count)
        {
            std::uint64_t struct_constant = 0x3bf9114e884521b8ULL;
            h(struct_constant);
            h(static_cast< std::uint64_t >(member_count));
        }

        template < typename Field, typename H >
        constexpr void hash_struct_member(H& h, std::string_view field_name)
        {
            hash_type_string(h, field_name);
            rpnx::serial4::typesig< Field >::hash(h);
        }

        inline auto cpu_name(cpu value) -> std::string
        {
            switch (value)
            {
            case cpu::none:
                return "none";
            case cpu::x86_32:
                return "x86_32";
            case cpu::x86_64:
                return "x86_64";
            case cpu::arm_32:
                return "arm_32";
            case cpu::arm_64:
                return "arm_64";
            case cpu::riscv_32:
                return "riscv_32";
            case cpu::riscv_64:
                return "riscv_64";
            }

            return "unknown";
        }

        inline auto os_name(os value) -> std::string
        {
            switch (value)
            {
            case os::none:
                return "none";
            case os::linux:
                return "linux";
            case os::windows:
                return "windows";
            case os::macos:
                return "macos";
            case os::freebsd:
                return "freebsd";
            case os::netbsd:
                return "netbsd";
            case os::openbsd:
                return "openbsd";
            case os::solaris:
                return "solaris";
            }

            return "unknown";
        }

        inline auto binary_name(binary value) -> std::string
        {
            switch (value)
            {
            case binary::none:
                return "none";
            case binary::elf:
                return "elf";
            case binary::macho:
                return "macho";
            case binary::pe:
                return "pe";
            case binary::wasm:
                return "wasm";
            }

            return "unknown";
        }

        template < typename Map >
        inline auto debug_key_list(Map const& map) -> std::string
        {
            std::string result = "[";
            bool first = true;

            for (auto const& [key, value] : map)
            {
                (void)value;

                if (!first)
                {
                    result += ", ";
                }
                else
                {
                    first = false;
                }

                result += key;
            }

            result += "]";
            return result;
        }

        inline auto output_info_debug_string(output_info const& value) -> std::string
        {
            std::string result = "output_info{cpu_type=";
            result += cpu_name(value.cpu_type);
            result += ", os_type=";
            result += os_name(value.os_type);
            result += ", binary_type=";
            result += binary_name(value.binary_type);
            result += "}";
            return result;
        }

        inline auto module_source_debug_string(module_source const& value) -> std::string
        {
            std::string result = "module_source{files=";
            result += debug_key_list(value.files);
            result += "}";
            return result;
        }

        inline auto module_source_name_map_debug_string(std::map< std::string, std::string > const& value) -> std::string
        {
            std::string result = "module_source_name_map{";
            bool first = true;

            for (auto const& [logical_name, source_name] : value)
            {
                if (!first)
                {
                    result += ", ";
                }
                else
                {
                    first = false;
                }

                result += logical_name;
                result += "=";
                result += source_name;
            }

            result += "}";
            return result;
        }

        inline auto source_bundle_debug_string(source_bundle const& value) -> std::string
        {
            std::string result = "source_bundle{targets=";
            result += debug_key_list(value.targets);
            result += ", module_sources=";
            result += debug_key_list(value.module_sources);
            result += "}";
            return result;
        }
    } // namespace detail

    template < typename T >
    inline auto querygraph_json_string(T const& value) -> std::string
    {
        std::vector< std::byte > bytes;
        rpnx::serial4::json_serialize_iter(value, std::back_inserter(bytes));
        return std::string(reinterpret_cast< char const* >(bytes.data()), bytes.size());
    }

    template < typename T >
    inline auto querygraph_serialize(T const& value) -> std::vector< std::byte >
    {
        std::vector< std::byte > bytes;
        rpnx::serial4::serialize_iter(value, std::back_inserter(bytes));
        return bytes;
    }
} // namespace quxlang

namespace rpnx
{
    template < typename T >
    concept serial4_debuggable = requires(T const& value, std::vector< std::byte > buffer) {
        rpnx::serial4::serialize_iter(value, std::back_inserter(buffer));
    };

    template < serial4_debuggable T >
    struct querygraph_debug_traits< T >
    {
        static auto to_debug_string(T const& value) -> std::string
        {
            (void)value;
            return std::string{"serial4{"} + typeid(T).name() + "}";
        }
    };

    template <>
    struct enum_traits< quxlang::cpu >
    {
        static auto constexpr strings()
        {
            return std::vector< std::string >{"none", "x86_32", "x86_64", "arm_32", "arm_64", "riscv_32", "riscv_64"};
        }

        static auto constexpr to_string(quxlang::cpu value) -> std::string
        {
            return strings().at(static_cast< std::size_t >(value));
        }

        static auto constexpr from_string(std::string_view value) -> quxlang::cpu
        {
            auto const values = strings();

            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if (values[index] == value)
                {
                    return static_cast< quxlang::cpu >(index);
                }
            }

            throw std::runtime_error("invalid cpu enum string");
        }
    };

    template <>
    struct enum_traits< quxlang::os >
    {
        static auto constexpr strings()
        {
            return std::vector< std::string >{"none", "linux", "windows", "macos", "freebsd", "netbsd", "openbsd", "solaris"};
        }

        static auto constexpr to_string(quxlang::os value) -> std::string
        {
            return strings().at(static_cast< std::size_t >(value));
        }

        static auto constexpr from_string(std::string_view value) -> quxlang::os
        {
            auto const values = strings();

            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if (values[index] == value)
                {
                    return static_cast< quxlang::os >(index);
                }
            }

            throw std::runtime_error("invalid os enum string");
        }
    };

    template <>
    struct enum_traits< quxlang::binary >
    {
        static auto constexpr strings()
        {
            return std::vector< std::string >{"none", "elf", "macho", "pe", "wasm"};
        }

        static auto constexpr to_string(quxlang::binary value) -> std::string
        {
            return strings().at(static_cast< std::size_t >(value));
        }

        static auto constexpr from_string(std::string_view value) -> quxlang::binary
        {
            auto const values = strings();

            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if (values[index] == value)
                {
                    return static_cast< quxlang::binary >(index);
                }
            }

            throw std::runtime_error("invalid binary enum string");
        }
    };

    template <>
    struct enum_traits< quxlang::output_kind >
    {
        static auto constexpr strings()
        {
            return std::vector< std::string >{"executable", "shared_library", "static_library", "image"};
        }

        static auto constexpr to_string(quxlang::output_kind value) -> std::string
        {
            return strings().at(static_cast< std::size_t >(value));
        }

        static auto constexpr from_string(std::string_view value) -> quxlang::output_kind
        {
            auto const values = strings();

            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if (values[index] == value)
                {
                    return static_cast< quxlang::output_kind >(index);
                }
            }

            throw std::runtime_error("invalid output_kind enum string");
        }
    };

    template <>
    struct querygraph_traits< quxlang::output_info >
    {
        static auto serialize_to_binary(quxlang::output_info const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct querygraph_traits< quxlang::module_source >
    {
        static auto serialize_to_binary(quxlang::module_source const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct querygraph_traits< std::map< std::string, std::string > >
    {
        static auto serialize_to_binary(std::map< std::string, std::string > const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct querygraph_traits< quxlang::source_bundle >
    {
        static auto serialize_to_binary(quxlang::source_bundle const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct querygraph_debug_traits< quxlang::output_info >
    {
        static auto to_debug_string(quxlang::output_info const& value) -> std::string
        {
            return quxlang::detail::output_info_debug_string(value);
        }
    };

    template <>
    struct querygraph_debug_traits< quxlang::module_source >
    {
        static auto to_debug_string(quxlang::module_source const& value) -> std::string
        {
            return quxlang::detail::module_source_debug_string(value);
        }
    };

    template <>
    struct querygraph_debug_traits< std::map< std::string, std::string > >
    {
        static auto to_debug_string(std::map< std::string, std::string > const& value) -> std::string
        {
            return quxlang::detail::module_source_name_map_debug_string(value);
        }
    };

    template <>
    struct querygraph_debug_traits< quxlang::source_bundle >
    {
        static auto to_debug_string(quxlang::source_bundle const& value) -> std::string
        {
            return quxlang::detail::source_bundle_debug_string(value);
        }
    };
} // namespace rpnx

namespace rpnx::serial4
{
    template <>
    class binary_serial_traits< quxlang::output_info >
    {
      public:
        template < typename It >
        static auto constexpr serialize_iter(quxlang::output_info const& value, It output) -> It
        {
            output = rpnx::serial4::serialize_iter(value.cpu_type, output);
            output = rpnx::serial4::serialize_iter(value.os_type, output);
            output = rpnx::serial4::serialize_iter(value.binary_type, output);
            return output;
        }

        template < typename It >
        static auto constexpr serialize_iter(quxlang::output_info const& value, It output, It end) -> It
        {
            output = rpnx::serial4::serialize_iter(value.cpu_type, output, end);
            output = rpnx::serial4::serialize_iter(value.os_type, output, end);
            output = rpnx::serial4::serialize_iter(value.binary_type, output, end);
            return output;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::output_info& value, It input) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.cpu_type, input);
            input = rpnx::serial4::deserialize_iter(value.os_type, input);
            input = rpnx::serial4::deserialize_iter(value.binary_type, input);
            return input;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::output_info& value, It input, It end) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.cpu_type, input, end);
            input = rpnx::serial4::deserialize_iter(value.os_type, input, end);
            input = rpnx::serial4::deserialize_iter(value.binary_type, input, end);
            return input;
        }
    };

    template <>
    class binary_serial_traits< quxlang::output_config >
    {
      public:
        template < typename It >
        static auto constexpr serialize_iter(quxlang::output_config const& value, It output) -> It
        {
            output = rpnx::serial4::serialize_iter(value.type, output);
            output = rpnx::serial4::serialize_iter(value.module, output);
            output = rpnx::serial4::serialize_iter(value.main_functanoid, output);
            return output;
        }

        template < typename It >
        static auto constexpr serialize_iter(quxlang::output_config const& value, It output, It end) -> It
        {
            output = rpnx::serial4::serialize_iter(value.type, output, end);
            output = rpnx::serial4::serialize_iter(value.module, output, end);
            output = rpnx::serial4::serialize_iter(value.main_functanoid, output, end);
            return output;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::output_config& value, It input) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.type, input);
            input = rpnx::serial4::deserialize_iter(value.module, input);
            input = rpnx::serial4::deserialize_iter(value.main_functanoid, input);
            return input;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::output_config& value, It input, It end) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.type, input, end);
            input = rpnx::serial4::deserialize_iter(value.module, input, end);
            input = rpnx::serial4::deserialize_iter(value.main_functanoid, input, end);
            return input;
        }
    };

    template <>
    class binary_serial_traits< quxlang::target_configuration >
    {
      public:
        template < typename It >
        static auto constexpr serialize_iter(quxlang::target_configuration const& value, It output) -> It
        {
            output = rpnx::serial4::serialize_iter(value.module_configurations, output);
            output = rpnx::serial4::serialize_iter(value.target_output_config, output);
            output = rpnx::serial4::serialize_iter(value.outputs, output);
            return output;
        }

        template < typename It >
        static auto constexpr serialize_iter(quxlang::target_configuration const& value, It output, It end) -> It
        {
            output = rpnx::serial4::serialize_iter(value.module_configurations, output, end);
            output = rpnx::serial4::serialize_iter(value.target_output_config, output, end);
            output = rpnx::serial4::serialize_iter(value.outputs, output, end);
            return output;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::target_configuration& value, It input) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.module_configurations, input);
            input = rpnx::serial4::deserialize_iter(value.target_output_config, input);
            input = rpnx::serial4::deserialize_iter(value.outputs, input);
            return input;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::target_configuration& value, It input, It end) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.module_configurations, input, end);
            input = rpnx::serial4::deserialize_iter(value.target_output_config, input, end);
            input = rpnx::serial4::deserialize_iter(value.outputs, input, end);
            return input;
        }
    };

    template <>
    class binary_serial_traits< quxlang::source_bundle >
    {
      public:
        template < typename It >
        static auto constexpr serialize_iter(quxlang::source_bundle const& value, It output) -> It
        {
            output = rpnx::serial4::serialize_iter(value.targets, output);
            output = rpnx::serial4::serialize_iter(value.module_sources, output);
            return output;
        }

        template < typename It >
        static auto constexpr serialize_iter(quxlang::source_bundle const& value, It output, It end) -> It
        {
            output = rpnx::serial4::serialize_iter(value.targets, output, end);
            output = rpnx::serial4::serialize_iter(value.module_sources, output, end);
            return output;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::source_bundle& value, It input) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.targets, input);
            input = rpnx::serial4::deserialize_iter(value.module_sources, input);
            return input;
        }

        template < typename It >
        static auto constexpr deserialize_iter(quxlang::source_bundle& value, It input, It end) -> It
        {
            input = rpnx::serial4::deserialize_iter(value.targets, input, end);
            input = rpnx::serial4::deserialize_iter(value.module_sources, input, end);
            return input;
        }
    };

    template < typename T >
    class typesig< rpnx::cow< T > >
    {
      public:
        template < typename H >
        static constexpr void hash(H& h)
        {
            quxlang::detail::hash_struct_prefix(h, 1);
            quxlang::detail::hash_struct_member< T >(h, "cow_value");
        }
    };

    template <>
    class typesig< quxlang::output_info >
    {
      public:
        template < typename H >
        static constexpr void hash(H& h)
        {
            quxlang::detail::hash_struct_prefix(h, 3);
            quxlang::detail::hash_struct_member< quxlang::cpu >(h, "cpu_type");
            quxlang::detail::hash_struct_member< quxlang::os >(h, "os_type");
            quxlang::detail::hash_struct_member< quxlang::binary >(h, "binary_type");
        }
    };

    template <>
    class typesig< quxlang::output_config >
    {
      public:
        template < typename H >
        static constexpr void hash(H& h)
        {
            quxlang::detail::hash_struct_prefix(h, 3);
            quxlang::detail::hash_struct_member< quxlang::output_kind >(h, "type");
            quxlang::detail::hash_struct_member< std::optional< std::string > >(h, "module");
            quxlang::detail::hash_struct_member< std::optional< std::string > >(h, "main_functanoid");
        }
    };

    template <>
    class typesig< quxlang::target_configuration >
    {
      public:
        template < typename H >
        static constexpr void hash(H& h)
        {
            quxlang::detail::hash_struct_prefix(h, 3);
            quxlang::detail::hash_struct_member< std::map< std::string, quxlang::module_configuration > >(h, "module_configurations");
            quxlang::detail::hash_struct_member< quxlang::output_info >(h, "target_output_config");
            quxlang::detail::hash_struct_member< std::map< std::string, quxlang::output_config > >(h, "outputs");
        }
    };

    template <>
    class typesig< quxlang::source_bundle >
    {
      public:
        template < typename H >
        static constexpr void hash(H& h)
        {
            quxlang::detail::hash_struct_prefix(h, 2);
            quxlang::detail::hash_struct_member< std::map< std::string, quxlang::target_configuration > >(h, "targets");
            quxlang::detail::hash_struct_member< std::map< std::string, quxlang::module_source > >(h, "module_sources");
        }
    };
} // namespace rpnx::serial4

#endif // QUXLANG_QUERIES_QUERYGRAPH_TRAITS_HEADER_GUARD
