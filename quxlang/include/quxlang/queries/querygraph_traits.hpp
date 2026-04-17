// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_QUERYGRAPH_TRAITS_HEADER_GUARD
#define QUXLANG_QUERIES_QUERYGRAPH_TRAITS_HEADER_GUARD

#include <quxlang/data/machine.hpp>
#include <quxlang/data/target_configuration.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/assembler.hpp>

#include <rpnx/querygraph/querygraph.hpp>
#include <rpnx/serialization4.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace quxlang
{
    namespace detail
    {
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

        inline auto source_file_name_debug_string(source_file_name const& value) -> std::string
        {
            return value.source_module + "/" + value.relative_path;
        }

        inline auto source_file_index_debug_string(source_file_index const& value) -> std::string
        {
            std::string result = "source_file_index{";
            bool first = true;
            for (auto const& [id, file] : value.id_to_file)
            {
                if (!first)
                {
                    result += ", ";
                }
                first = false;
                result += std::to_string(id);
                result += "=";
                result += source_file_name_debug_string(file);
            }
            result += "}";
            return result;
        }

        inline auto module_options_map_debug_string(std::map< std::string, std::map< std::string, std::string > > const& value) -> std::string
        {
            std::string result = "module_option_strings_map{";
            bool first_module = true;

            for (auto const& [module_name, option_values] : value)
            {
                if (!first_module)
                {
                    result += ", ";
                }
                else
                {
                    first_module = false;
                }

                result += module_name;
                result += "=";
                result += debug_key_list(option_values);
            }

            result += "}";
            return result;
        }

        inline auto module_options_map_debug_string(std::map< type_symbol, std::string > const& value) -> std::string
        {
            std::string result = "module_options_map{";
            bool first = true;

            for (auto const& [option_symbol, option_value] : value)
            {
                if (!first)
                {
                    result += ", ";
                }
                else
                {
                    first = false;
                }

                result += to_string(option_symbol);
                result += "=";
                result += option_value;
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

    template < typename T >
    concept querygraph_string_debuggable = requires(T const& value) {
        { quxlang::to_string(value) } -> std::convertible_to< std::string >;
    };

    template < typename T >
    concept querygraph_json_debuggable = requires(T const& value, std::vector< std::byte > bytes) {
        rpnx::serial4::json_serialize_iter(value, std::back_inserter(bytes));
    };
} // namespace quxlang

namespace rpnx
{
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
} // namespace rpnx

namespace rpnx::querygraph
{
    template < quxlang::querygraph_string_debuggable T >
    struct debug_traits< T >
    {
        static auto to_debug_string(T const& value) -> std::string
        {
            return quxlang::to_string(value);
        }
    };

    template <>
    struct debug_traits< quxlang::vmir2::functanoid_routine3 >
    {
        static auto to_debug_string(quxlang::vmir2::functanoid_routine3 const& value) -> std::string
        {
            quxlang::vmir2::assembler assembler(value);
            return assembler.to_string(value);
        }
    };

    template < typename T >
        requires (!quxlang::querygraph_string_debuggable< T > && quxlang::querygraph_json_debuggable< T >)
    struct debug_traits< T >
    {
        static auto to_debug_string(T const& value) -> std::string
        {
            return quxlang::querygraph_json_string(value);
        }
    };

    template <>
    struct binary_traits< quxlang::output_info >
    {
        static auto serialize_to_binary(quxlang::output_info const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct binary_traits< quxlang::module_source >
    {
        static auto serialize_to_binary(quxlang::module_source const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct binary_traits< std::map< std::string, std::string > >
    {
        static auto serialize_to_binary(std::map< std::string, std::string > const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct binary_traits< std::map< std::string, std::map< std::string, std::string > > >
    {
        static auto serialize_to_binary(std::map< std::string, std::map< std::string, std::string > > const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct binary_traits< std::map< quxlang::type_symbol, std::string > >
    {
        static auto serialize_to_binary(std::map< quxlang::type_symbol, std::string > const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct binary_traits< quxlang::source_bundle >
    {
        static auto serialize_to_binary(quxlang::source_bundle const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct binary_traits< quxlang::source_file_name >
    {
        static auto serialize_to_binary(quxlang::source_file_name const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct binary_traits< quxlang::source_file_index >
    {
        static auto serialize_to_binary(quxlang::source_file_index const& value) -> std::vector< std::byte >
        {
            return quxlang::querygraph_serialize(value);
        }
    };

    template <>
    struct debug_traits< quxlang::output_info >
    {
        static auto to_debug_string(quxlang::output_info const& value) -> std::string
        {
            return quxlang::detail::output_info_debug_string(value);
        }
    };

    template <>
    struct debug_traits< quxlang::module_source >
    {
        static auto to_debug_string(quxlang::module_source const& value) -> std::string
        {
            return quxlang::detail::module_source_debug_string(value);
        }
    };

    template <>
    struct debug_traits< std::map< std::string, std::string > >
    {
        static auto to_debug_string(std::map< std::string, std::string > const& value) -> std::string
        {
            return quxlang::detail::module_source_name_map_debug_string(value);
        }
    };

    template <>
    struct debug_traits< std::map< std::string, std::map< std::string, std::string > > >
    {
        static auto to_debug_string(std::map< std::string, std::map< std::string, std::string > > const& value) -> std::string
        {
            return quxlang::detail::module_options_map_debug_string(value);
        }
    };

    template <>
    struct debug_traits< std::map< quxlang::type_symbol, std::string > >
    {
        static auto to_debug_string(std::map< quxlang::type_symbol, std::string > const& value) -> std::string
        {
            return quxlang::detail::module_options_map_debug_string(value);
        }
    };

    template <>
    struct debug_traits< quxlang::source_bundle >
    {
        static auto to_debug_string(quxlang::source_bundle const& value) -> std::string
        {
            return quxlang::detail::source_bundle_debug_string(value);
        }
    };

    template <>
    struct debug_traits< quxlang::source_file_name >
    {
        static auto to_debug_string(quxlang::source_file_name const& value) -> std::string
        {
            return quxlang::detail::source_file_name_debug_string(value);
        }
    };

    template <>
    struct debug_traits< quxlang::source_file_index >
    {
        static auto to_debug_string(quxlang::source_file_index const& value) -> std::string
        {
            return quxlang::detail::source_file_index_debug_string(value);
        }
    };
} // namespace rpnx::querygraph

namespace rpnx::serial4
{
    template < typename T >
    struct typesig_hashing_traits< rpnx::cow< T > >
    {
        template < typename H >
        static constexpr void partial_hash(detail::typesig16_hashing_context& context, H& h)
        {
            detail::hash_typesig_with_recursion_guard< rpnx::cow< T > >(context, h, [&]() {
                constexpr std::uint64_t struct_constant = 0x3bf9114e884521b8ULL;
                h(struct_constant);
                h(std::uint64_t{1});
                detail::partial_hash_string(h, "cow_value");
                detail::partial_hash_with_context< T >(context, h);
            });
        }
    };
} // namespace rpnx::serial4

#endif // QUXLANG_QUERIES_QUERYGRAPH_TRAITS_HEADER_GUARD
