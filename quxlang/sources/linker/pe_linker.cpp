// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/linker/pe_linker.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <utility>

namespace
{
    constexpr std::uint16_t machine_i386 = 0x014c;
    constexpr std::uint16_t machine_amd64 = 0x8664;
    constexpr std::uint16_t machine_arm64 = 0xaa64;

    constexpr std::uint32_t section_code = 0x00000020;
    constexpr std::uint32_t section_initialized = 0x00000040;
    constexpr std::uint32_t section_uninitialized = 0x00000080;
    constexpr std::uint32_t section_remove = 0x00000800;
    constexpr std::uint32_t section_memory_execute = 0x20000000;
    constexpr std::uint32_t section_memory_read = 0x40000000;
    constexpr std::uint32_t section_memory_write = 0x80000000;

    auto align_up(std::uint64_t value, std::uint64_t alignment) -> std::uint64_t
    {
        return alignment == 0 ? value : (value + alignment - 1) / alignment * alignment;
    }

    struct byte_reader
    {
        std::vector< std::byte > const& bytes;

        void require(std::size_t offset, std::size_t size, std::string_view what) const
        {
            if (offset > bytes.size() || size > bytes.size() - offset)
            {
                throw quxlang::semantic_compilation_error("Malformed COFF object while reading " + std::string(what));
            }
        }

        auto u8(std::size_t offset) const -> std::uint8_t
        {
            require(offset, 1, "byte");
            return std::to_integer< std::uint8_t >(bytes[offset]);
        }

        auto u16(std::size_t offset) const -> std::uint16_t
        {
            require(offset, 2, "16-bit value");
            return std::uint16_t(u8(offset)) | (std::uint16_t(u8(offset + 1)) << 8);
        }

        auto i16(std::size_t offset) const -> std::int16_t { return static_cast< std::int16_t >(u16(offset)); }

        auto u32(std::size_t offset) const -> std::uint32_t
        {
            require(offset, 4, "32-bit value");
            return std::uint32_t(u8(offset)) | (std::uint32_t(u8(offset + 1)) << 8) | (std::uint32_t(u8(offset + 2)) << 16) |
                   (std::uint32_t(u8(offset + 3)) << 24);
        }

        auto bytes_at(std::size_t offset, std::size_t size) const -> std::vector< std::byte >
        {
            require(offset, size, "section contents");
            return std::vector< std::byte >(bytes.begin() + static_cast< std::ptrdiff_t >(offset), bytes.begin() + static_cast< std::ptrdiff_t >(offset + size));
        }
    };

    template < typename T > void append_le(std::vector< std::byte >& output, T value)
    {
        using U = std::make_unsigned_t< T >;
        U const bits = static_cast< U >(value);
        for (std::size_t i = 0; i < sizeof(T); ++i)
        {
            output.push_back(static_cast< std::byte >((bits >> (i * 8)) & U(0xff)));
        }
    }

    template < typename T > void put_le(std::vector< std::byte >& output, std::size_t offset, T value)
    {
        if (offset > output.size() || sizeof(T) > output.size() - offset)
        {
            throw quxlang::compiler_bug("PE linker attempted an out-of-range write");
        }
        using U = std::make_unsigned_t< T >;
        U const bits = static_cast< U >(value);
        for (std::size_t i = 0; i < sizeof(T); ++i)
        {
            output[offset + i] = static_cast< std::byte >((bits >> (i * 8)) & U(0xff));
        }
    }

    auto read_u32(std::vector< std::byte > const& bytes, std::size_t offset) -> std::uint32_t
    {
        if (offset > bytes.size() || 4 > bytes.size() - offset)
        {
            throw quxlang::semantic_compilation_error("COFF relocation is outside its section");
        }
        return std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset])) |
               (std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset + 1])) << 8) |
               (std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset + 2])) << 16) |
               (std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset + 3])) << 24);
    }

    auto read_u64(std::vector< std::byte > const& bytes, std::size_t offset) -> std::uint64_t
    {
        std::uint64_t result = 0;
        for (std::size_t i = 0; i < 8; ++i)
        {
            if (offset + i >= bytes.size()) throw quxlang::semantic_compilation_error("COFF relocation is outside its section");
            result |= std::uint64_t(std::to_integer< std::uint8_t >(bytes[offset + i])) << (i * 8);
        }
        return result;
    }

    auto sign_extend(std::uint64_t value, unsigned bits) -> std::int64_t
    {
        std::uint64_t const sign = std::uint64_t(1) << (bits - 1);
        return static_cast< std::int64_t >((value ^ sign) - sign);
    }

    struct coff_relocation
    {
        std::uint32_t offset = 0;
        std::uint32_t symbol_index = 0;
        std::uint16_t type = 0;
    };

    struct coff_section
    {
        std::string name;
        std::vector< std::byte > contents;
        std::vector< coff_relocation > relocations;
        std::uint32_t logical_size = 0;
        std::uint32_t characteristics = 0;
    };

    struct coff_symbol
    {
        std::string name;
        std::uint32_t value = 0;
        std::int16_t section = 0;
        bool valid = false;
    };

    struct parsed_coff
    {
        std::uint16_t machine = 0;
        std::vector< coff_section > sections;
        std::vector< coff_symbol > symbols;
    };

    auto string_from_fixed(byte_reader const& reader, std::size_t offset, std::size_t size) -> std::string
    {
        reader.require(offset, size, "name");
        std::string result;
        for (std::size_t i = 0; i < size && reader.u8(offset + i) != 0; ++i) result.push_back(static_cast< char >(reader.u8(offset + i)));
        return result;
    }

    auto parse_coff(std::vector< std::byte > const& bytes) -> parsed_coff
    {
        byte_reader const reader{bytes};
        reader.require(0, 20, "COFF header");
        parsed_coff result;
        result.machine = reader.u16(0);
        std::uint16_t const section_count = reader.u16(2);
        std::uint32_t const symbol_offset = reader.u32(8);
        std::uint32_t const symbol_count = reader.u32(12);
        std::uint16_t const optional_header_size = reader.u16(16);
        std::size_t const section_table = 20 + optional_header_size;
        reader.require(section_table, std::size_t(section_count) * 40, "COFF section table");

        std::size_t string_table = 0;
        std::uint32_t string_table_size = 0;
        if (symbol_count != 0)
        {
            reader.require(symbol_offset, std::size_t(symbol_count) * 18 + 4, "COFF symbol and string tables");
            string_table = std::size_t(symbol_offset) + std::size_t(symbol_count) * 18;
            string_table_size = reader.u32(string_table);
            if (string_table_size < 4) throw quxlang::semantic_compilation_error("Malformed COFF string table");
            reader.require(string_table, string_table_size, "COFF string table");
        }

        auto long_name = [&](std::uint32_t offset) -> std::string
        {
            if (string_table == 0 || offset < 4 || offset >= string_table_size) throw quxlang::semantic_compilation_error("Invalid COFF string-table name");
            std::string value;
            for (std::size_t cursor = string_table + offset; cursor < string_table + string_table_size && reader.u8(cursor) != 0; ++cursor)
                value.push_back(static_cast< char >(reader.u8(cursor)));
            return value;
        };

        result.sections.reserve(section_count);
        for (std::uint16_t i = 0; i < section_count; ++i)
        {
            std::size_t const header = section_table + std::size_t(i) * 40;
            std::string name = string_from_fixed(reader, header, 8);
            if (name.starts_with('/') && name.size() > 1)
            {
                std::uint32_t offset = 0;
                for (char c : name.substr(1))
                {
                    if (c < '0' || c > '9') throw quxlang::semantic_compilation_error("Invalid long COFF section name");
                    offset = offset * 10 + std::uint32_t(c - '0');
                }
                name = long_name(offset);
            }
            std::uint32_t const raw_size = reader.u32(header + 16);
            std::uint32_t const raw_offset = reader.u32(header + 20);
            std::uint32_t const relocation_offset = reader.u32(header + 24);
            std::uint16_t const relocation_count = reader.u16(header + 32);
            coff_section section{
                .name = std::move(name),
                .contents = raw_size == 0 ? std::vector< std::byte >{} : reader.bytes_at(raw_offset, raw_size),
                .logical_size = std::max(reader.u32(header + 8), raw_size),
                .characteristics = reader.u32(header + 36),
            };
            reader.require(relocation_offset, std::size_t(relocation_count) * 10, "COFF relocations");
            for (std::uint16_t relocation = 0; relocation < relocation_count; ++relocation)
            {
                std::size_t const position = relocation_offset + std::size_t(relocation) * 10;
                section.relocations.push_back(coff_relocation{reader.u32(position), reader.u32(position + 4), reader.u16(position + 8)});
            }
            result.sections.push_back(std::move(section));
        }

        result.symbols.resize(symbol_count);
        for (std::uint32_t i = 0; i < symbol_count;)
        {
            std::size_t const position = std::size_t(symbol_offset) + std::size_t(i) * 18;
            std::string name;
            if (reader.u32(position) == 0) name = long_name(reader.u32(position + 4));
            else name = string_from_fixed(reader, position, 8);
            std::uint8_t const auxiliary_count = reader.u8(position + 17);
            if (std::uint64_t(i) + auxiliary_count >= symbol_count) throw quxlang::semantic_compilation_error("Malformed COFF auxiliary symbol count");
            result.symbols[i] = coff_symbol{std::move(name), reader.u32(position + 8), reader.i16(position + 12), true};
            i += std::uint32_t(auxiliary_count) + 1;
        }
        return result;
    }

    struct output_section
    {
        std::string name;
        std::vector< std::byte > contents;
        std::uint32_t logical_size = 0;
        std::uint32_t characteristics = 0;
        std::uint32_t rva = 0;
        std::uint32_t raw_offset = 0;
        bool entirely_uninitialized = false;
    };

    struct section_placement
    {
        std::size_t output_index = 0;
        std::uint32_t offset = 0;
        bool included = false;
    };

    auto input_alignment(std::uint32_t characteristics) -> std::uint32_t
    {
        std::uint32_t const encoded = (characteristics >> 20) & 0xf;
        return encoded == 0 ? 16 : std::uint32_t(1) << (encoded - 1);
    }

    auto output_group_name(coff_section const& section) -> std::string
    {
        if (section.name.starts_with(".pdata")) return ".pdata";
        if (section.name.starts_with(".tls")) return ".tls";
        if ((section.characteristics & (section_code | section_memory_execute)) != 0) return ".text";
        if ((section.characteristics & section_memory_write) != 0)
            return (section.characteristics & section_uninitialized) != 0 && section.contents.empty() ? ".bss" : ".data";
        return ".rdata";
    }

    auto include_section(coff_section const& section) -> bool
    {
        if ((section.characteristics & section_remove) != 0 || section.name == ".drectve" || section.name == ".llvm_addrsig") return false;
        return (section.characteristics & (section_code | section_initialized | section_uninitialized | section_memory_read | section_memory_write | section_memory_execute)) != 0;
    }

    struct import_layout
    {
        std::vector< std::byte > contents;
        std::map< std::string, std::uint32_t > iat_rvas;
        std::uint32_t iat_start = 0;
        std::uint32_t iat_size = 0;
    };

    auto build_import_section(std::vector< quxlang::pe_dynamic_import > const& imports, std::uint32_t section_rva, std::size_t pointer_size) -> import_layout
    {
        std::map< std::string, std::vector< std::string > > libraries;
        for (quxlang::pe_dynamic_import const& import : imports)
        {
            if (import.optional) throw quxlang::semantic_compilation_error("Optional Windows imports are not supported by static PE import tables: " + import.symbol_name);
            std::string library = import.library_name;
            if (!library.ends_with(".dll") && !library.ends_with(".DLL")) library += ".dll";
            libraries[library].push_back(import.symbol_name);
        }
        for (auto& [library, names] : libraries)
        {
            std::sort(names.begin(), names.end());
            names.erase(std::unique(names.begin(), names.end()), names.end());
        }

        import_layout result;
        result.contents.resize((libraries.size() + 1) * 20);
        std::map< std::string, std::uint32_t > ilt_offsets;
        std::map< std::string, std::uint32_t > iat_offsets;
        for (auto const& [library, names] : libraries)
        {
            ilt_offsets[library] = static_cast< std::uint32_t >(result.contents.size());
            result.contents.resize(result.contents.size() + (names.size() + 1) * pointer_size);
        }
        result.iat_start = section_rva + static_cast< std::uint32_t >(result.contents.size());
        for (auto const& [library, names] : libraries)
        {
            iat_offsets[library] = static_cast< std::uint32_t >(result.contents.size());
            result.contents.resize(result.contents.size() + (names.size() + 1) * pointer_size);
        }
        result.iat_size = section_rva + static_cast< std::uint32_t >(result.contents.size()) - result.iat_start;

        std::map< std::string, std::uint32_t > library_name_offsets;
        std::map< std::pair< std::string, std::string >, std::uint32_t > hint_name_offsets;
        for (auto const& [library, names] : libraries)
        {
            library_name_offsets[library] = static_cast< std::uint32_t >(result.contents.size());
            for (char c : library) result.contents.push_back(static_cast< std::byte >(c));
            result.contents.push_back(std::byte{0});
            if ((result.contents.size() & 1) != 0) result.contents.push_back(std::byte{0});
            for (std::string const& name : names)
            {
                hint_name_offsets[{library, name}] = static_cast< std::uint32_t >(result.contents.size());
                append_le< std::uint16_t >(result.contents, 0);
                for (char c : name) result.contents.push_back(static_cast< std::byte >(c));
                result.contents.push_back(std::byte{0});
                if ((result.contents.size() & 1) != 0) result.contents.push_back(std::byte{0});
            }
        }

        std::size_t descriptor = 0;
        for (auto const& [library, names] : libraries)
        {
            put_le< std::uint32_t >(result.contents, descriptor, section_rva + ilt_offsets.at(library));
            put_le< std::uint32_t >(result.contents, descriptor + 12, section_rva + library_name_offsets.at(library));
            put_le< std::uint32_t >(result.contents, descriptor + 16, section_rva + iat_offsets.at(library));
            for (std::size_t i = 0; i < names.size(); ++i)
            {
                std::uint64_t const hint_name_rva = section_rva + hint_name_offsets.at({library, names[i]});
                if (pointer_size == 8)
                {
                    put_le< std::uint64_t >(result.contents, ilt_offsets.at(library) + i * 8, hint_name_rva);
                    put_le< std::uint64_t >(result.contents, iat_offsets.at(library) + i * 8, hint_name_rva);
                }
                else
                {
                    put_le< std::uint32_t >(result.contents, ilt_offsets.at(library) + i * 4, static_cast< std::uint32_t >(hint_name_rva));
                    put_le< std::uint32_t >(result.contents, iat_offsets.at(library) + i * 4, static_cast< std::uint32_t >(hint_name_rva));
                }
                std::uint32_t const iat_rva = section_rva + iat_offsets.at(library) + static_cast< std::uint32_t >(i * pointer_size);
                result.iat_rvas.emplace("__imp_" + names[i], iat_rva);
            }
            descriptor += 20;
        }
        return result;
    }

    auto build_base_relocations(std::vector< std::pair< std::uint32_t, std::uint16_t > > relocations) -> std::vector< std::byte >
    {
        std::sort(relocations.begin(), relocations.end());
        relocations.erase(std::unique(relocations.begin(), relocations.end()), relocations.end());
        std::vector< std::byte > result;
        std::size_t cursor = 0;
        while (cursor < relocations.size())
        {
            std::uint32_t const page = relocations[cursor].first & ~std::uint32_t(0xfff);
            std::size_t const block_start = result.size();
            append_le< std::uint32_t >(result, page);
            append_le< std::uint32_t >(result, 0);
            while (cursor < relocations.size() && (relocations[cursor].first & ~std::uint32_t(0xfff)) == page)
            {
                append_le< std::uint16_t >(result, std::uint16_t((relocations[cursor].second << 12) | (relocations[cursor].first - page)));
                ++cursor;
            }
            if ((result.size() - block_start) % 4 != 0) append_le< std::uint16_t >(result, 0);
            put_le< std::uint32_t >(result, block_start + 4, static_cast< std::uint32_t >(result.size() - block_start));
        }
        return result;
    }
} // namespace

auto quxlang::pe_linker::link_windows_executable(machine_target_info const& machine,
                                                  std::vector< std::byte > const& object_file,
                                                  std::string const& entry_symbol,
                                                  pe_link_options const& options) const -> std::vector< std::byte >
{
    if (machine.os_type != os::windows || machine.binary_type != binary::pe)
        throw semantic_compilation_error("PE linker requires a Windows PE target");

    std::uint16_t expected_machine = 0;
    bool pe32_plus = true;
    switch (machine.cpu_type)
    {
    case cpu::x86_32: expected_machine = machine_i386; pe32_plus = false; break;
    case cpu::x86_64: expected_machine = machine_amd64; break;
    case cpu::arm_64: expected_machine = machine_arm64; break;
    default: throw semantic_compilation_error("Windows PE executable linking is not implemented for this CPU");
    }

    parsed_coff const input = parse_coff(object_file);
    if (input.machine != expected_machine) throw semantic_compilation_error("COFF object machine does not match the Windows target CPU");

    std::vector< output_section > output_sections;
    std::map< std::string, std::size_t > group_indices;
    std::vector< section_placement > placements(input.sections.size());
    auto find_or_add_group = [&](std::string const& name, std::uint32_t characteristics, bool uninitialized) -> std::size_t
    {
        auto const found = group_indices.find(name);
        if (found != group_indices.end())
        {
            output_sections[found->second].characteristics |= characteristics;
            output_sections[found->second].entirely_uninitialized &= uninitialized;
            return found->second;
        }
        std::size_t const index = output_sections.size();
        group_indices.emplace(name, index);
        output_sections.push_back(output_section{.name = name, .characteristics = characteristics, .entirely_uninitialized = uninitialized});
        return index;
    };

    for (std::size_t i = 0; i < input.sections.size(); ++i)
    {
        coff_section const& section = input.sections[i];
        if (!include_section(section) || (section.logical_size == 0 && section.contents.empty())) continue;
        std::string const group_name = output_group_name(section);
        bool const uninitialized = section.contents.empty() && (section.characteristics & section_uninitialized) != 0;
        std::uint32_t characteristics = section.characteristics & (section_code | section_initialized | section_uninitialized | section_memory_execute | section_memory_read | section_memory_write);
        if ((characteristics & section_memory_read) == 0) characteristics |= section_memory_read;
        std::size_t const group = find_or_add_group(group_name, characteristics, uninitialized);
        output_section& target = output_sections[group];
        std::uint32_t const offset = static_cast< std::uint32_t >(align_up(target.logical_size, input_alignment(section.characteristics)));
        target.logical_size = offset + std::max< std::uint32_t >(section.logical_size, static_cast< std::uint32_t >(section.contents.size()));
        if (!uninitialized)
        {
            target.contents.resize(std::max< std::size_t >(target.contents.size(), offset + section.contents.size()));
            std::copy(section.contents.begin(), section.contents.end(), target.contents.begin() + offset);
        }
        placements[i] = section_placement{group, offset, true};
    }

    // COFF common symbols reserve zero-initialized storage whose size is stored in Value.
    std::map< std::uint32_t, std::pair< std::size_t, std::uint32_t > > common_symbols;
    for (std::uint32_t i = 0; i < input.symbols.size(); ++i)
    {
        coff_symbol const& symbol = input.symbols[i];
        if (!symbol.valid || symbol.section != 0 || symbol.value == 0) continue;
        std::size_t const bss = find_or_add_group(".bss", section_uninitialized | section_memory_read | section_memory_write, true);
        std::uint32_t alignment = 1;
        while (alignment < symbol.value && alignment < 16) alignment *= 2;
        std::uint32_t const offset = static_cast< std::uint32_t >(align_up(output_sections[bss].logical_size, alignment));
        output_sections[bss].logical_size = offset + symbol.value;
        common_symbols.emplace(i, std::make_pair(bss, offset));
    }

    std::optional< std::size_t > import_section_index;
    std::map< std::string, std::pair< std::size_t, std::uint32_t > > import_thunks;
    std::vector< std::pair< std::uint32_t, std::uint16_t > > import_thunk_relocations;
    if (!options.dynamic_imports.empty())
    {
        if (input.machine == machine_amd64 || input.machine == machine_i386)
        {
            std::size_t const text_section = find_or_add_group(".text", section_code | section_memory_execute | section_memory_read, false);
            output_section& text = output_sections[text_section];
            for (quxlang::pe_dynamic_import const& import : options.dynamic_imports)
            {
                if (import_thunks.contains(import.symbol_name))
                {
                    continue;
                }
                std::uint32_t const offset = static_cast< std::uint32_t >(align_up(text.contents.size(), 16));
                text.contents.resize(offset + 6);
                text.contents[offset] = std::byte{0xff};
                text.contents[offset + 1] = std::byte{0x25};
                text.logical_size = static_cast< std::uint32_t >(text.contents.size());
                import_thunks.emplace(import.symbol_name, std::make_pair(text_section, offset));
            }
        }
        import_section_index = output_sections.size();
        import_layout const initial = build_import_section(options.dynamic_imports, 0, pe32_plus ? 8 : 4);
        output_sections.push_back(output_section{
            .name = ".idata", .contents = initial.contents, .logical_size = static_cast< std::uint32_t >(initial.contents.size()),
            .characteristics = section_initialized | section_memory_read | section_memory_write});
    }

    std::uint32_t constexpr section_alignment = 0x1000;
    std::uint32_t constexpr file_alignment = 0x200;
    std::uint64_t const image_base = pe32_plus ? UINT64_C(0x140000000) : UINT64_C(0x400000);
    std::uint32_t next_rva = section_alignment;
    for (output_section& section : output_sections)
    {
        section.rva = next_rva;
        next_rva = static_cast< std::uint32_t >(align_up(std::uint64_t(section.rva) + section.logical_size, section_alignment));
    }

    import_layout imports;
    if (import_section_index.has_value())
    {
        output_section& section = output_sections[*import_section_index];
        imports = build_import_section(options.dynamic_imports, section.rva, pe32_plus ? 8 : 4);
        section.contents = imports.contents;
        section.logical_size = static_cast< std::uint32_t >(section.contents.size());

        // I386 COFF decorates imported names (for example __imp__ExitProcess@4).
        // Bind those object-level spellings to the undecorated import-table entry.
        for (coff_symbol const& symbol : input.symbols)
        {
            if (!symbol.valid || symbol.section != 0 || !symbol.name.starts_with("__imp_")) continue;
            std::string undecorated = symbol.name.substr(6);
            if (undecorated.starts_with('_')) undecorated.erase(undecorated.begin());
            std::size_t const suffix = undecorated.rfind('@');
            if (suffix != std::string::npos && suffix + 1 < undecorated.size() &&
                std::all_of(undecorated.begin() + static_cast< std::ptrdiff_t >(suffix + 1), undecorated.end(), [](unsigned char c) { return c >= '0' && c <= '9'; }))
            {
                undecorated.resize(suffix);
            }
            auto const canonical = imports.iat_rvas.find("__imp_" + undecorated);
            if (canonical != imports.iat_rvas.end()) imports.iat_rvas.emplace(symbol.name, canonical->second);
        }

        for (auto const& [symbol_name, placement] : import_thunks)
        {
            auto const imported = imports.iat_rvas.find("__imp_" + symbol_name);
            if (imported == imports.iat_rvas.end())
            {
                throw semantic_compilation_error("Missing IAT slot for Windows import thunk '" + symbol_name + "'");
            }
            output_section& section = output_sections[placement.first];
            std::uint32_t const thunk_rva = section.rva + placement.second;
            if (input.machine == machine_amd64)
            {
                std::int64_t const displacement = static_cast< std::int64_t >(imported->second) - static_cast< std::int64_t >(thunk_rva + 6);
                if (displacement < std::numeric_limits< std::int32_t >::min() || displacement > std::numeric_limits< std::int32_t >::max())
                {
                    throw semantic_compilation_error("Windows x64 import thunk displacement is out of range");
                }
                put_le< std::uint32_t >(section.contents, placement.second + 2, static_cast< std::uint32_t >(displacement));
            }
            else
            {
                put_le< std::uint32_t >(section.contents, placement.second + 2, static_cast< std::uint32_t >(image_base + imported->second));
                import_thunk_relocations.emplace_back(thunk_rva + 2, 3);
            }
        }
    }

    auto symbol_location = [&](std::uint32_t index) -> std::pair< std::uint64_t, std::pair< std::size_t, std::uint32_t > >
    {
        if (index >= input.symbols.size() || !input.symbols[index].valid) throw semantic_compilation_error("COFF relocation references an invalid symbol");
        coff_symbol const& symbol = input.symbols[index];
        auto const common = common_symbols.find(index);
        if (common != common_symbols.end())
        {
            output_section const& section = output_sections[common->second.first];
            return {image_base + section.rva + common->second.second, common->second};
        }
        if (symbol.section > 0)
        {
            std::size_t const input_index = static_cast< std::size_t >(symbol.section - 1);
            if (input_index >= placements.size() || !placements[input_index].included)
                throw semantic_compilation_error("COFF symbol '" + symbol.name + "' belongs to a discarded section");
            section_placement const placement = placements[input_index];
            std::uint32_t const offset = placement.offset + symbol.value;
            return {image_base + output_sections[placement.output_index].rva + offset, {placement.output_index, offset}};
        }
        if (symbol.section == -1) return {symbol.value, {std::numeric_limits< std::size_t >::max(), symbol.value}};
        auto thunk = import_thunks.find(symbol.name);
        if (thunk == import_thunks.end() && input.machine == machine_i386)
        {
            std::string undecorated = symbol.name;
            if (undecorated.starts_with('_')) undecorated.erase(undecorated.begin());
            std::size_t const suffix = undecorated.rfind('@');
            if (suffix != std::string::npos && suffix + 1 < undecorated.size() &&
                std::all_of(undecorated.begin() + static_cast< std::ptrdiff_t >(suffix + 1), undecorated.end(), [](unsigned char c) { return c >= '0' && c <= '9'; }))
            {
                undecorated.resize(suffix);
            }
            thunk = import_thunks.find(undecorated);
        }
        if (thunk != import_thunks.end())
        {
            output_section const& section = output_sections[thunk->second.first];
            return {image_base + section.rva + thunk->second.second, thunk->second};
        }
        auto const imported = imports.iat_rvas.find(symbol.name);
        if (imported != imports.iat_rvas.end())
            return {image_base + imported->second, {*import_section_index, imported->second - output_sections[*import_section_index].rva}};
        throw semantic_compilation_error("Unresolved Windows COFF symbol '" + symbol.name + "'");
    };

    std::vector< std::pair< std::uint32_t, std::uint16_t > > base_relocations = std::move(import_thunk_relocations);
    for (std::size_t input_index = 0; input_index < input.sections.size(); ++input_index)
    {
        if (!placements[input_index].included) continue;
        section_placement const placement = placements[input_index];
        output_section& target_section = output_sections[placement.output_index];
        for (coff_relocation const& relocation : input.sections[input_index].relocations)
        {
            std::size_t const patch = std::size_t(placement.offset) + relocation.offset;
            std::uint64_t const patch_va = image_base + target_section.rva + patch;
            std::uint32_t const patch_rva = target_section.rva + static_cast< std::uint32_t >(patch);
            auto const [symbol_va, symbol_place] = symbol_location(relocation.symbol_index);
            std::uint64_t const symbol_rva = symbol_va - image_base;
            if (input.machine == machine_amd64)
            {
                if (relocation.type == 0x0001)
                {
                    put_le< std::uint64_t >(target_section.contents, patch, symbol_va + read_u64(target_section.contents, patch));
                    base_relocations.emplace_back(patch_rva, 10);
                }
                else if (relocation.type == 0x0002) put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(symbol_va + read_u32(target_section.contents, patch)));
                else if (relocation.type == 0x0003) put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(symbol_rva + read_u32(target_section.contents, patch)));
                else if (relocation.type >= 0x0004 && relocation.type <= 0x0009)
                {
                    std::int64_t const addend = static_cast< std::int32_t >(read_u32(target_section.contents, patch));
                    std::int64_t const value = static_cast< std::int64_t >(symbol_va) + addend - static_cast< std::int64_t >(patch_va + 4 + (relocation.type - 4));
                    if (value < INT32_MIN || value > INT32_MAX) throw semantic_compilation_error("AMD64 COFF relative relocation is out of range");
                    put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(value));
                }
                else if (relocation.type == 0x000a) put_le< std::uint16_t >(target_section.contents, patch, static_cast< std::uint16_t >(symbol_place.first + 1));
                else if (relocation.type == 0x000b) put_le< std::uint32_t >(target_section.contents, patch, symbol_place.second + read_u32(target_section.contents, patch));
                else throw semantic_compilation_error("Unsupported AMD64 COFF relocation type " + std::to_string(relocation.type));
            }
            else if (input.machine == machine_i386)
            {
                if (relocation.type == 0x0006)
                {
                    put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(symbol_va + read_u32(target_section.contents, patch)));
                    base_relocations.emplace_back(patch_rva, 3);
                }
                else if (relocation.type == 0x0007) put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(symbol_rva + read_u32(target_section.contents, patch)));
                else if (relocation.type == 0x000a) put_le< std::uint16_t >(target_section.contents, patch, static_cast< std::uint16_t >(symbol_place.first + 1));
                else if (relocation.type == 0x000b) put_le< std::uint32_t >(target_section.contents, patch, symbol_place.second + read_u32(target_section.contents, patch));
                else if (relocation.type == 0x0014)
                {
                    std::int64_t const value = static_cast< std::int64_t >(symbol_va) + static_cast< std::int32_t >(read_u32(target_section.contents, patch)) - static_cast< std::int64_t >(patch_va + 4);
                    if (value < INT32_MIN || value > INT32_MAX) throw semantic_compilation_error("I386 COFF relative relocation is out of range");
                    put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(value));
                }
                else throw semantic_compilation_error("Unsupported I386 COFF relocation type " + std::to_string(relocation.type));
            }
            else
            {
                std::uint32_t instruction = read_u32(target_section.contents, patch);
                if (relocation.type == 0x0001) put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(symbol_va + instruction));
                else if (relocation.type == 0x0002) put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(symbol_rva + instruction));
                else if (relocation.type == 0x000e)
                {
                    put_le< std::uint64_t >(target_section.contents, patch, symbol_va + read_u64(target_section.contents, patch));
                    base_relocations.emplace_back(patch_rva, 10);
                }
                else if (relocation.type == 0x0003)
                {
                    std::int64_t const displacement = static_cast< std::int64_t >(symbol_va) - static_cast< std::int64_t >(patch_va);
                    if ((displacement & 3) != 0 || displacement < -(INT64_C(1) << 27) || displacement >= (INT64_C(1) << 27))
                        throw semantic_compilation_error("ARM64 branch relocation is out of range");
                    instruction = (instruction & 0xfc000000U) | (static_cast< std::uint32_t >(displacement >> 2) & 0x03ffffffU);
                    put_le< std::uint32_t >(target_section.contents, patch, instruction);
                }
                else if (relocation.type == 0x0004)
                {
                    std::int64_t const displacement = static_cast< std::int64_t >(symbol_va & ~UINT64_C(0xfff)) - static_cast< std::int64_t >(patch_va & ~UINT64_C(0xfff));
                    std::uint32_t const immediate = static_cast< std::uint32_t >(displacement >> 12) & 0x1fffffU;
                    instruction = (instruction & 0x9f00001fU) | ((immediate & 3U) << 29) | ((immediate >> 2) << 5);
                    put_le< std::uint32_t >(target_section.contents, patch, instruction);
                }
                else if (relocation.type == 0x0006 || relocation.type == 0x0007)
                {
                    std::uint32_t scale = relocation.type == 0x0007 ? ((instruction >> 30) & 3U) : 0;
                    std::uint32_t const immediate = (static_cast< std::uint32_t >(symbol_va) & 0xfffU) >> scale;
                    instruction = (instruction & ~(0xfffU << 10)) | ((immediate & 0xfffU) << 10);
                    put_le< std::uint32_t >(target_section.contents, patch, instruction);
                }
                else if (relocation.type == 0x0008) put_le< std::uint32_t >(target_section.contents, patch, symbol_place.second + instruction);
                else if (relocation.type == 0x000d) put_le< std::uint16_t >(target_section.contents, patch, static_cast< std::uint16_t >(symbol_place.first + 1));
                else if (relocation.type == 0x0011)
                {
                    std::int64_t const value = static_cast< std::int64_t >(symbol_va) + static_cast< std::int32_t >(instruction) - static_cast< std::int64_t >(patch_va + 4);
                    put_le< std::uint32_t >(target_section.contents, patch, static_cast< std::uint32_t >(value));
                }
                else throw semantic_compilation_error("Unsupported ARM64 COFF relocation type " + std::to_string(relocation.type));
            }
        }
    }

    std::optional< std::size_t > relocation_section_index;
    if (!base_relocations.empty())
    {
        relocation_section_index = output_sections.size();
        std::vector< std::byte > contents = build_base_relocations(std::move(base_relocations));
        output_sections.push_back(output_section{.name = ".reloc", .contents = std::move(contents), .characteristics = section_initialized | section_memory_read});
        output_sections.back().logical_size = static_cast< std::uint32_t >(output_sections.back().contents.size());
        output_sections.back().rva = next_rva;
        next_rva = static_cast< std::uint32_t >(align_up(std::uint64_t(next_rva) + output_sections.back().logical_size, section_alignment));
    }

    auto find_symbol = [&](std::string const& name) -> std::optional< std::uint32_t >
    {
        for (std::uint32_t i = 0; i < input.symbols.size(); ++i)
            if (input.symbols[i].valid && input.symbols[i].name == name && input.symbols[i].section != 0) return i;
        return std::nullopt;
    };
    std::optional< std::uint32_t > entry_index = find_symbol(entry_symbol);
    if (!entry_index.has_value() && input.machine == machine_i386) entry_index = find_symbol("_" + entry_symbol);
    if (!entry_index.has_value()) throw semantic_compilation_error("Windows PE entry symbol is not defined: " + entry_symbol);
    std::uint32_t const entry_rva = static_cast< std::uint32_t >(symbol_location(*entry_index).first - image_base);

    if (output_sections.empty() || output_sections.size() > 96) throw semantic_compilation_error("Windows PE output has an unsupported number of sections");
    std::uint32_t const pe_offset = 0x80;
    std::uint16_t const optional_size = pe32_plus ? 240 : 224;
    std::uint32_t const headers_size = static_cast< std::uint32_t >(align_up(pe_offset + 4 + 20 + optional_size + output_sections.size() * 40, file_alignment));
    std::uint32_t next_raw = headers_size;
    for (output_section& section : output_sections)
    {
        if (section.entirely_uninitialized) continue;
        section.raw_offset = next_raw;
        next_raw += static_cast< std::uint32_t >(align_up(section.contents.size(), file_alignment));
    }

    std::vector< std::byte > result(next_raw);
    result[0] = std::byte{'M'}; result[1] = std::byte{'Z'};
    put_le< std::uint32_t >(result, 0x3c, pe_offset);
    char const stub[] = "This program cannot be run in DOS mode.\r\n$";
    std::copy(reinterpret_cast< std::byte const* >(stub), reinterpret_cast< std::byte const* >(stub + sizeof(stub) - 1), result.begin() + 0x40);
    result[pe_offset] = std::byte{'P'}; result[pe_offset + 1] = std::byte{'E'};
    std::size_t const coff_header = pe_offset + 4;
    put_le< std::uint16_t >(result, coff_header, expected_machine);
    put_le< std::uint16_t >(result, coff_header + 2, static_cast< std::uint16_t >(output_sections.size()));
    put_le< std::uint16_t >(result, coff_header + 16, optional_size);
    std::uint16_t characteristics = 0x0002 | 0x0020;
    if (!pe32_plus) characteristics |= 0x0100;
    if (!relocation_section_index.has_value()) characteristics |= 0x0001;
    put_le< std::uint16_t >(result, coff_header + 18, characteristics);

    std::size_t const optional = coff_header + 20;
    put_le< std::uint16_t >(result, optional, pe32_plus ? 0x020b : 0x010b);
    std::uint32_t size_code = 0, size_initialized = 0, size_uninitialized = 0, base_code = 0, base_data = 0;
    for (output_section const& section : output_sections)
    {
        std::uint32_t const raw_size = section.entirely_uninitialized ? 0 : static_cast< std::uint32_t >(align_up(section.contents.size(), file_alignment));
        if ((section.characteristics & section_code) != 0)
        {
            size_code += raw_size;
            if (base_code == 0) base_code = section.rva;
        }
        else if (section.entirely_uninitialized) size_uninitialized += static_cast< std::uint32_t >(align_up(section.logical_size, section_alignment));
        else
        {
            size_initialized += raw_size;
            if (base_data == 0) base_data = section.rva;
        }
    }
    put_le< std::uint32_t >(result, optional + 4, size_code);
    put_le< std::uint32_t >(result, optional + 8, size_initialized);
    put_le< std::uint32_t >(result, optional + 12, size_uninitialized);
    put_le< std::uint32_t >(result, optional + 16, entry_rva);
    put_le< std::uint32_t >(result, optional + 20, base_code);
    if (pe32_plus) put_le< std::uint64_t >(result, optional + 24, image_base);
    else
    {
        put_le< std::uint32_t >(result, optional + 24, base_data);
        put_le< std::uint32_t >(result, optional + 28, static_cast< std::uint32_t >(image_base));
    }
    std::size_t const windows_fields = optional + (pe32_plus ? 32 : 32);
    put_le< std::uint32_t >(result, windows_fields, section_alignment);
    put_le< std::uint32_t >(result, windows_fields + 4, file_alignment);
    put_le< std::uint16_t >(result, windows_fields + 8, 6);
    put_le< std::uint16_t >(result, windows_fields + 16, 6);
    put_le< std::uint32_t >(result, windows_fields + 24, next_rva);
    put_le< std::uint32_t >(result, windows_fields + 28, headers_size);
    put_le< std::uint16_t >(result, windows_fields + 36, 3); // console subsystem
    std::uint16_t dll_characteristics = 0x0100; // NX compatible
    if (relocation_section_index.has_value()) dll_characteristics |= 0x0040;
    if (pe32_plus) dll_characteristics |= 0x0020;
    put_le< std::uint16_t >(result, windows_fields + 38, dll_characteristics);
    if (pe32_plus)
    {
        put_le< std::uint64_t >(result, windows_fields + 40, UINT64_C(0x100000));
        put_le< std::uint64_t >(result, windows_fields + 48, UINT64_C(0x1000));
        put_le< std::uint64_t >(result, windows_fields + 56, UINT64_C(0x100000));
        put_le< std::uint64_t >(result, windows_fields + 64, UINT64_C(0x1000));
        put_le< std::uint32_t >(result, windows_fields + 76, 16);
    }
    else
    {
        put_le< std::uint32_t >(result, windows_fields + 40, 0x100000);
        put_le< std::uint32_t >(result, windows_fields + 44, 0x1000);
        put_le< std::uint32_t >(result, windows_fields + 48, 0x100000);
        put_le< std::uint32_t >(result, windows_fields + 52, 0x1000);
        put_le< std::uint32_t >(result, windows_fields + 60, 16);
    }
    std::size_t const directories = optional + (pe32_plus ? 112 : 96);
    if (import_section_index.has_value())
    {
        output_section const& section = output_sections[*import_section_index];
        put_le< std::uint32_t >(result, directories + 8, section.rva);
        // The import directory ends at the null descriptor, before ILT data.
        std::set< std::string > libraries;
        for (pe_dynamic_import const& import : options.dynamic_imports) libraries.insert(import.library_name);
        put_le< std::uint32_t >(result, directories + 12, static_cast< std::uint32_t >((libraries.size() + 1) * 20));
        put_le< std::uint32_t >(result, directories + 12 * 8, imports.iat_start);
        put_le< std::uint32_t >(result, directories + 12 * 8 + 4, imports.iat_size);
    }
    if (group_indices.contains(".pdata"))
    {
        output_section const& section = output_sections[group_indices.at(".pdata")];
        put_le< std::uint32_t >(result, directories + 3 * 8, section.rva);
        put_le< std::uint32_t >(result, directories + 3 * 8 + 4, section.logical_size);
    }
    if (relocation_section_index.has_value())
    {
        output_section const& section = output_sections[*relocation_section_index];
        put_le< std::uint32_t >(result, directories + 5 * 8, section.rva);
        put_le< std::uint32_t >(result, directories + 5 * 8 + 4, section.logical_size);
    }

    std::size_t section_header = optional + optional_size;
    for (output_section const& section : output_sections)
    {
        for (std::size_t i = 0; i < section.name.size() && i < 8; ++i) result[section_header + i] = static_cast< std::byte >(section.name[i]);
        put_le< std::uint32_t >(result, section_header + 8, section.logical_size);
        put_le< std::uint32_t >(result, section_header + 12, section.rva);
        std::uint32_t const raw_size = section.entirely_uninitialized ? 0 : static_cast< std::uint32_t >(align_up(section.contents.size(), file_alignment));
        put_le< std::uint32_t >(result, section_header + 16, raw_size);
        put_le< std::uint32_t >(result, section_header + 20, section.raw_offset);
        put_le< std::uint32_t >(result, section_header + 36, section.characteristics);
        if (!section.entirely_uninitialized) std::copy(section.contents.begin(), section.contents.end(), result.begin() + section.raw_offset);
        section_header += 40;
    }
    return result;
}
