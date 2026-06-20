// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
// ai-generated file
// hopefully works okay

#include <quxlang/linker/elf_linker.hpp>

#include <quxlang/data/compilation_result.hpp>

#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace quxlang::detail
{
    namespace llvm = ::llvm;

    /**
     * linked_section stores one allocated input section after qxc linker layout.
     */
    struct linked_section
    {
        std::string name;
        std::uint32_t section_name_offset = 0;
        std::uint64_t input_index = 0;
        std::uint32_t section_type = llvm::ELF::SHT_PROGBITS;
        std::uint32_t section_link = 0;
        std::uint32_t section_info = 0;
        std::uint64_t entry_size = 0;
        std::uint64_t alignment = 1;
        bool allocated = true;
        bool writable = false;
        bool executable = false;
        bool tls = false;
        bool nobits = false;
        bool synthetic = false;
        std::vector< std::byte > contents;
        std::uint64_t memory_size = 0;
        std::uint64_t file_offset = 0;
        std::uint64_t virtual_address = 0;
    };

    /**
     * load_segment stores one output PT_LOAD program header.
     */
    struct load_segment
    {
        std::uint64_t file_offset = 0;
        std::uint64_t virtual_address = 0;
        std::uint64_t file_size = 0;
        std::uint64_t memory_size = 0;
        std::uint32_t flags = 0;
    };

    /**
     * tls_segment stores the output PT_TLS image bounds after section layout.
     */
    struct tls_segment
    {
        std::uint64_t file_offset = 0;
        std::uint64_t virtual_address = 0;
        std::uint64_t file_size = 0;
        std::uint64_t memory_size = 0;
        std::uint64_t alignment = 1;
    };

    struct got_slot
    {
        std::uint64_t target_address = 0;
        std::size_t slot_index = 0;
    };

    struct output_symbol
    {
        std::string name;
        std::uint64_t value = 0;
        std::uint64_t size = 0;
        std::uint8_t binding = 0;
        std::uint8_t type = 0;
        std::uint8_t other = 0;
        std::uint16_t section_index = 0;
    };

    struct common_symbol_allocation
    {
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
        std::uint64_t alignment = 1;
    };

    class elf_link_session
    {
    public:
        elf_link_session(quxlang::machine_target_info const& machine_info,
                         std::vector< std::byte > const& object_file_bytes,
                         std::string entry_symbol_name,
                         quxlang::elf_link_options link_options)
            : machine(machine_info),
              object_bytes(object_file_bytes),
              entry_symbol(std::move(entry_symbol_name)),
              options(link_options)
        {
        }

        auto link() -> std::vector< std::byte >
        {
            validate_machine();
            open_object();
            collect_sections();
            collect_common_symbols();
            layout_sections();
            collect_arm64_got_slots();
            if (got_section_index.has_value())
            {
                layout_sections();
                refresh_arm64_got_slots();
            }
            apply_relocations();
            if (options.preserve_symbols)
            {
                add_symbol_sections();
                layout_sections();
            }
            return build_executable_image();
        }

    private:
        quxlang::machine_target_info machine;
        std::vector< std::byte > const& object_bytes;
        std::string entry_symbol;
        quxlang::elf_link_options options;
        std::unique_ptr< llvm::MemoryBuffer > object_buffer;
        std::unique_ptr< llvm::object::ObjectFile > object_file;
        std::vector< linked_section > sections;
        std::vector< load_segment > load_segments;
        std::optional< tls_segment > tls_segment_info;
        std::vector< std::byte > section_name_table;
        std::map< std::uint64_t, std::size_t > output_section_indices_by_input_index;
        std::map< std::string, common_symbol_allocation > common_symbol_allocations;
        std::map< std::uint64_t, got_slot > got_slots_by_target_address;
        std::optional< std::size_t > common_bss_section_index;
        std::optional< std::size_t > got_section_index;
        std::uint64_t executable_base_address = 0;
        std::uint64_t page_alignment = 0x1000;
        std::uint64_t file_size = 0;
        std::uint64_t memory_size = 0;
        std::uint64_t section_name_table_file_offset = 0;
        std::uint32_t section_name_table_name_offset = 0;
        std::uint64_t section_header_offset = 0;
        std::uint16_t program_header_count = 0;
        std::uint16_t section_header_count = 0;
        std::uint16_t section_header_string_table_index = 0;

        template < typename T >
        auto take_or_throw(llvm::Expected< T > value, std::string const& context) const -> T
        {
            if (value)
            {
                return std::move(*value);
            }

            std::string error_text;
            llvm::handleAllErrors(value.takeError(), [&](llvm::ErrorInfoBase const& info)
            {
                error_text = info.message();
            });
            throw quxlang::semantic_compilation_error(context + ": " + error_text);
        }

        void validate_machine() const
        {
            if (machine.os_type != quxlang::os::linux || machine.binary_type != quxlang::binary::elf)
            {
                throw quxlang::semantic_compilation_error("elf_linker only supports Linux ELF outputs");
            }

            switch (machine.cpu_type)
            {
            case quxlang::cpu::x86_32:
                return;
            case quxlang::cpu::x86_64:
                return;
            case quxlang::cpu::arm_64:
                return;
            default:
                break;
            }

            throw quxlang::semantic_compilation_error("elf_linker does not support this Linux CPU target yet");
        }

        void open_object()
        {
            std::string const object_text(reinterpret_cast< char const* >(object_bytes.data()), object_bytes.size());
            object_buffer = llvm::MemoryBuffer::getMemBufferCopy(object_text, "qxc-link-input");
            llvm::Expected< std::unique_ptr< llvm::object::ObjectFile > > object_or_error = llvm::object::ObjectFile::createObjectFile(object_buffer->getMemBufferRef());
            if (!object_or_error)
            {
                std::string error_text;
                llvm::handleAllErrors(object_or_error.takeError(), [&](llvm::ErrorInfoBase const& info)
                {
                    error_text = info.message();
                });
                throw quxlang::semantic_compilation_error("Failed to open relocatable ELF object for linking: " + error_text);
            }

            if (!llvm::isa< llvm::object::ELFObjectFileBase >(object_or_error->get()))
            {
                throw quxlang::semantic_compilation_error("ELF linker received a non-ELF object file");
            }

            object_file = std::move(*object_or_error);
        }

        auto include_alloc_section(llvm::object::ELFSectionRef const& section, std::string const& name) const -> bool
        {
            if ((section.getFlags() & llvm::ELF::SHF_ALLOC) == 0)
            {
                return false;
            }

            if (name.rfind(".debug", 0) == 0)
            {
                return false;
            }

            if (name == ".eh_frame")
            {
                return false;
            }

            if (name == ".note.GNU-stack")
            {
                return false;
            }

            return true;
        }

        void collect_sections()
        {
            sections.clear();
            output_section_indices_by_input_index.clear();

            for (llvm::object::SectionRef const& generic_section : object_file->sections())
            {
                llvm::object::ELFSectionRef const section(generic_section);
                std::string const name = take_or_throw(generic_section.getName(), "Failed to read ELF section name").str();
                if (!include_alloc_section(section, name))
                {
                    continue;
                }

                linked_section output_section;
                output_section.name = name;
                output_section.input_index = generic_section.getIndex();
                output_section.section_type = section.getType();
                output_section.alignment = std::max< std::uint64_t >(1, generic_section.getAlignment().value());
                output_section.allocated = true;
                output_section.writable = (section.getFlags() & llvm::ELF::SHF_WRITE) != 0;
                output_section.executable = (section.getFlags() & llvm::ELF::SHF_EXECINSTR) != 0;
                output_section.tls = (section.getFlags() & llvm::ELF::SHF_TLS) != 0;
                output_section.nobits = section.getType() == llvm::ELF::SHT_NOBITS;
                output_section.synthetic = false;
                output_section.memory_size = generic_section.getSize();
                if (!output_section.nobits)
                {
                    llvm::StringRef const contents = take_or_throw(generic_section.getContents(), "Failed to read ELF section contents");
                    output_section.contents.resize(contents.size());
                    std::memcpy(output_section.contents.data(), contents.data(), contents.size());
                }

                output_section_indices_by_input_index[output_section.input_index] = sections.size();
                sections.push_back(std::move(output_section));
            }
        }

        auto is_common_symbol(llvm::object::SymbolRef const& symbol) const -> bool
        {
            std::uint32_t const flags = take_or_throw(symbol.getFlags(), "Failed to read ELF symbol flags");
            return (flags & llvm::object::BasicSymbolRef::SF_Common) != 0;
        }

        auto symbol_name(llvm::object::SymbolRef const& symbol) const -> std::string
        {
            return take_or_throw(symbol.getName(), "Failed to read ELF symbol name").str();
        }

        void collect_common_symbols()
        {
            common_symbol_allocations.clear();
            common_bss_section_index.reset();

            std::uint64_t offset = 0;
            std::uint64_t max_alignment = 1;
            for (llvm::object::SymbolRef const& generic_symbol : object_file->symbols())
            {
                if (!is_common_symbol(generic_symbol))
                {
                    continue;
                }

                llvm::object::ELFSymbolRef const symbol(generic_symbol);
                std::uint64_t const size = symbol.getSize();
                std::uint64_t const alignment = std::max< std::uint64_t >(1, take_or_throw(generic_symbol.getAddress(), "Failed to read ELF common symbol alignment"));
                offset = align_up(offset, alignment);
                common_symbol_allocations.emplace(
                    symbol_name(generic_symbol),
                    common_symbol_allocation{
                        .offset = offset,
                        .size = size,
                        .alignment = alignment,
                    });
                offset += size;
                max_alignment = std::max(max_alignment, alignment);
            }

            if (offset == 0)
            {
                return;
            }

            linked_section common_bss;
            common_bss.name = ".lbss";
            common_bss.input_index = static_cast< std::uint64_t >(sections.size() + 1);
            common_bss.section_type = llvm::ELF::SHT_NOBITS;
            common_bss.alignment = max_alignment;
            common_bss.allocated = true;
            common_bss.writable = true;
            common_bss.executable = false;
            common_bss.nobits = true;
            common_bss.synthetic = true;
            common_bss.memory_size = offset;
            common_bss_section_index = sections.size();
            sections.push_back(std::move(common_bss));
        }

        auto align_up(std::uint64_t value, std::uint64_t alignment) const -> std::uint64_t
        {
            if (alignment <= 1)
            {
                return value;
            }
            std::uint64_t const mask = alignment - 1;
            return (value + mask) & ~mask;
        }

        auto section_program_flags(linked_section const& section) const -> std::uint32_t
        {
            std::uint32_t flags = llvm::ELF::PF_R;
            if (section.writable)
            {
                flags |= llvm::ELF::PF_W;
            }
            if (section.executable)
            {
                flags |= llvm::ELF::PF_X;
            }
            return flags;
        }

        auto count_loadable_section_runs() const -> std::uint16_t
        {
            std::uint16_t count = 0;
            std::optional< std::uint32_t > current_flags;
            auto count_section = [&](linked_section const& section)
            {
                if (!section.allocated)
                {
                    return;
                }
                std::uint32_t const flags = section_program_flags(section);
                if (!current_flags.has_value() || *current_flags != flags)
                {
                    ++count;
                    current_flags = flags;
                }
            };

            for (linked_section const& section : sections)
            {
                if (!section.nobits)
                {
                    count_section(section);
                }
            }

            for (linked_section const& section : sections)
            {
                if (section.nobits)
                {
                    count_section(section);
                }
            }

            return count;
        }

        /**
         * Returns true when any allocated output section participates in the static TLS block.
         */
        auto has_tls_sections() const -> bool
        {
            for (linked_section const& section : sections)
            {
                if (section.allocated && section.tls)
                {
                    return true;
                }
            }
            return false;
        }

        /**
         * Computes the output TLS program-header bounds from laid-out TLS sections.
         */
        void compute_tls_segment()
        {
            tls_segment_info.reset();

            std::optional< std::uint64_t > start_virtual_address;
            std::uint64_t start_file_offset = 0;
            std::uint64_t initialized_end_virtual_address = 0;
            std::uint64_t memory_end_virtual_address = 0;
            std::uint64_t max_alignment = 1;

            for (linked_section const& section : sections)
            {
                if (!section.allocated || !section.tls)
                {
                    continue;
                }

                if (!start_virtual_address.has_value() || section.virtual_address < *start_virtual_address)
                {
                    start_virtual_address = section.virtual_address;
                    start_file_offset = section.file_offset;
                }

                max_alignment = std::max(max_alignment, section.alignment);
                memory_end_virtual_address = std::max(memory_end_virtual_address, section.virtual_address + section.memory_size);
                if (!section.nobits)
                {
                    initialized_end_virtual_address = std::max(initialized_end_virtual_address, section.virtual_address + section.contents.size());
                }
            }

            if (!start_virtual_address.has_value())
            {
                return;
            }

            std::uint64_t const file_size =
                initialized_end_virtual_address > *start_virtual_address ? initialized_end_virtual_address - *start_virtual_address : 0;
            tls_segment_info = tls_segment{
                .file_offset = start_file_offset,
                .virtual_address = *start_virtual_address,
                .file_size = file_size,
                .memory_size = memory_end_virtual_address - *start_virtual_address,
                .alignment = max_alignment,
            };
        }

        void rebuild_section_name_table()
        {
            section_name_table.clear();
            section_name_table.push_back(std::byte{0});
            for (linked_section& section : sections)
            {
                section.section_name_offset = static_cast< std::uint32_t >(section_name_table.size());
                for (char const c : section.name)
                {
                    section_name_table.push_back(static_cast< std::byte >(c));
                }
                section_name_table.push_back(std::byte{0});
            }
        }

        void layout_sections()
        {
            switch (machine.cpu_type)
            {
            case quxlang::cpu::x86_32:
                executable_base_address = 0x08048000;
                break;
            case quxlang::cpu::x86_64:
            case quxlang::cpu::arm_64:
                executable_base_address = 0x00400000;
                break;
            default:
                throw quxlang::semantic_compilation_error("Unsupported ELF executable base address target");
            }

            std::uint64_t const elf_header_size = machine.pointer_size_bytes() == 8 ? 64 : 52;
            std::uint64_t const program_header_size = machine.pointer_size_bytes() == 8 ? 56 : 32;
            std::uint64_t const section_header_entry_size = machine.pointer_size_bytes() == 8 ? 64 : 40;
            program_header_count = static_cast< std::uint16_t >(1 + count_loadable_section_runs() + (has_tls_sections() ? 1 : 0));
            std::uint64_t const header_end = elf_header_size + program_header_size * program_header_count;
            std::uint64_t file_cursor = align_up(header_end, page_alignment);
            std::uint64_t memory_cursor = file_cursor;

            load_segments.clear();
            load_segments.push_back(load_segment{
                .file_offset = 0,
                .virtual_address = executable_base_address,
                .file_size = header_end,
                .memory_size = header_end,
                .flags = llvm::ELF::PF_R,
            });
            std::optional< std::size_t > current_load_segment_index;

            auto begin_or_continue_segment = [&](std::uint32_t flags)
            {
                if (current_load_segment_index.has_value() && load_segments.at(*current_load_segment_index).flags == flags)
                {
                    return;
                }

                file_cursor = align_up(file_cursor, page_alignment);
                memory_cursor = align_up(memory_cursor, page_alignment);
                current_load_segment_index = load_segments.size();
                load_segments.push_back(load_segment{
                    .file_offset = file_cursor,
                    .virtual_address = executable_base_address + memory_cursor,
                    .file_size = 0,
                    .memory_size = 0,
                    .flags = flags,
                });
            };

            auto update_current_segment = [&]()
            {
                if (!current_load_segment_index.has_value())
                {
                    throw quxlang::semantic_compilation_error("ELF linker internal segment layout error");
                }

                load_segment& segment = load_segments.at(*current_load_segment_index);
                segment.file_size = file_cursor - segment.file_offset;
                segment.memory_size = memory_cursor - (segment.virtual_address - executable_base_address);
            };

            for (linked_section& section : sections)
            {
                if (!section.allocated || section.nobits)
                {
                    continue;
                }

                begin_or_continue_segment(section_program_flags(section));
                file_cursor = align_up(file_cursor, section.alignment);
                memory_cursor = align_up(memory_cursor, section.alignment);
                section.file_offset = file_cursor;
                section.virtual_address = executable_base_address + memory_cursor;
                file_cursor += section.contents.size();
                memory_cursor += section.memory_size;
                update_current_segment();
            }

            for (linked_section& section : sections)
            {
                if (!section.allocated || !section.nobits)
                {
                    continue;
                }

                begin_or_continue_segment(section_program_flags(section));
                memory_cursor = align_up(memory_cursor, section.alignment);
                section.file_offset = file_cursor;
                section.virtual_address = executable_base_address + memory_cursor;
                memory_cursor += section.memory_size;
                update_current_segment();
            }

            compute_tls_segment();
            memory_size = memory_cursor;
            for (linked_section& section : sections)
            {
                if (section.allocated)
                {
                    continue;
                }

                file_cursor = align_up(file_cursor, section.alignment);
                section.file_offset = file_cursor;
                section.virtual_address = 0;
                file_cursor += section.contents.size();
                section.memory_size = section.contents.size();
            }

            rebuild_section_name_table();
            section_name_table_name_offset = static_cast< std::uint32_t >(section_name_table.size());
            std::string const section_name_table_name = ".shstrtab";
            for (char const c : section_name_table_name)
            {
                section_name_table.push_back(static_cast< std::byte >(c));
            }
            section_name_table.push_back(std::byte{0});

            section_name_table_file_offset = file_cursor;
            file_cursor += section_name_table.size();
            section_header_offset = align_up(file_cursor, machine.pointer_size_bytes() == 8 ? 8 : 4);
            section_header_count = static_cast< std::uint16_t >(sections.size() + 2);
            section_header_string_table_index = static_cast< std::uint16_t >(sections.size() + 1);
            file_cursor = section_header_offset + section_header_count * section_header_entry_size;
            file_size = file_cursor;
        }

        auto section_flags(linked_section const& section) const -> std::uint64_t
        {
            if (!section.allocated)
            {
                return 0;
            }

            std::uint64_t flags = llvm::ELF::SHF_ALLOC;
            if (section.writable)
            {
                flags |= llvm::ELF::SHF_WRITE;
            }
            if (section.executable)
            {
                flags |= llvm::ELF::SHF_EXECINSTR;
            }
            if (section.tls)
            {
                flags |= llvm::ELF::SHF_TLS;
            }
            return flags;
        }

        auto symbol_output_section_index(llvm::object::SymbolRef const& symbol) const -> std::optional< std::uint16_t >
        {
            if (is_common_symbol(symbol))
            {
                if (!common_bss_section_index.has_value())
                {
                    throw quxlang::semantic_compilation_error("ELF common symbol has no allocated output section");
                }
                return static_cast< std::uint16_t >(*common_bss_section_index + 1);
            }

            std::uint32_t const flags = take_or_throw(symbol.getFlags(), "Failed to read ELF symbol flags");
            if ((flags & llvm::object::BasicSymbolRef::SF_Undefined) != 0)
            {
                return static_cast< std::uint16_t >(llvm::ELF::SHN_UNDEF);
            }

            if ((flags & llvm::object::BasicSymbolRef::SF_Absolute) != 0)
            {
                return static_cast< std::uint16_t >(llvm::ELF::SHN_ABS);
            }

            llvm::object::section_iterator const section_iter = take_or_throw(symbol.getSection(), "Failed to read ELF symbol section");
            if (section_iter == object_file->section_end())
            {
                return static_cast< std::uint16_t >(llvm::ELF::SHN_ABS);
            }

            std::map< std::uint64_t, std::size_t >::const_iterator output_section_iter = output_section_indices_by_input_index.find(section_iter->getIndex());
            if (output_section_iter == output_section_indices_by_input_index.end())
            {
                return std::nullopt;
            }

            return static_cast< std::uint16_t >(output_section_iter->second + 1);
        }

        auto symbol_output_value(llvm::object::SymbolRef const& symbol, std::uint16_t section_index) const -> std::uint64_t
        {
            if (section_index == llvm::ELF::SHN_UNDEF || section_index == llvm::ELF::SHN_ABS)
            {
                return take_or_throw(symbol.getAddress(), "Failed to read ELF symbol address");
            }

            return symbol_address(symbol);
        }

        auto build_output_symbols() const -> std::vector< output_symbol >
        {
            std::vector< output_symbol > result;
            for (llvm::object::SymbolRef const& generic_symbol : object_file->symbols())
            {
                std::optional< std::uint16_t > const section_index = symbol_output_section_index(generic_symbol);
                if (!section_index.has_value())
                {
                    continue;
                }

                llvm::object::ELFSymbolRef const symbol(generic_symbol);
                std::string symbol_name = take_or_throw(symbol.getName(), "Failed to read ELF symbol name").str();
                std::map< std::string, std::string >::const_iterator display_name = options.symbol_display_names.find(symbol_name);
                if (display_name != options.symbol_display_names.end())
                {
                    symbol_name = display_name->second;
                }

                result.push_back(output_symbol{
                    .name = std::move(symbol_name),
                    .value = symbol_output_value(symbol, *section_index),
                    .size = symbol.getSize(),
                    .binding = symbol.getBinding(),
                    .type = symbol.getELFType(),
                    .other = symbol.getOther(),
                    .section_index = *section_index,
                });
            }

            return result;
        }

        auto build_symbol_string_table(std::vector< output_symbol >& symbols) const -> std::vector< std::byte >
        {
            std::vector< std::byte > string_table;
            string_table.push_back(std::byte{0});
            for (output_symbol& symbol : symbols)
            {
                if (symbol.name.empty())
                {
                    continue;
                }

                for (char const c : symbol.name)
                {
                    string_table.push_back(static_cast< std::byte >(c));
                }
                string_table.push_back(std::byte{0});
            }
            return string_table;
        }

        void write_symbol_table_64(std::vector< std::byte >& symbol_table, std::size_t index, output_symbol const& symbol, std::uint32_t name_offset) const
        {
            std::size_t const offset = index * 24;
            write_u32(symbol_table, offset, name_offset);
            symbol_table[offset + 4] = static_cast< std::byte >((symbol.binding << 4) | (symbol.type & 0xf));
            symbol_table[offset + 5] = static_cast< std::byte >(symbol.other);
            write_u16(symbol_table, offset + 6, symbol.section_index);
            write_u64(symbol_table, offset + 8, symbol.value);
            write_u64(symbol_table, offset + 16, symbol.size);
        }

        void write_symbol_table_32(std::vector< std::byte >& symbol_table, std::size_t index, output_symbol const& symbol, std::uint32_t name_offset) const
        {
            std::size_t const offset = index * 16;
            write_u32(symbol_table, offset, name_offset);
            write_u32(symbol_table, offset + 4, static_cast< std::uint32_t >(symbol.value));
            write_u32(symbol_table, offset + 8, static_cast< std::uint32_t >(symbol.size));
            symbol_table[offset + 12] = static_cast< std::byte >((symbol.binding << 4) | (symbol.type & 0xf));
            symbol_table[offset + 13] = static_cast< std::byte >(symbol.other);
            write_u16(symbol_table, offset + 14, symbol.section_index);
        }

        auto build_symbol_table(std::vector< output_symbol > const& symbols) const -> std::vector< std::byte >
        {
            std::size_t const entry_size = machine.pointer_size_bytes() == 8 ? 24 : 16;
            std::vector< std::byte > symbol_table((symbols.size() + 1) * entry_size, std::byte{0});
            std::uint32_t name_offset = 1;
            for (std::size_t i = 0; i < symbols.size(); ++i)
            {
                output_symbol const& symbol = symbols.at(i);
                std::uint32_t const current_name_offset = symbol.name.empty() ? 0 : name_offset;
                if (machine.pointer_size_bytes() == 8)
                {
                    write_symbol_table_64(symbol_table, i + 1, symbol, current_name_offset);
                }
                else
                {
                    write_symbol_table_32(symbol_table, i + 1, symbol, current_name_offset);
                }

                if (!symbol.name.empty())
                {
                    name_offset += static_cast< std::uint32_t >(symbol.name.size() + 1);
                }
            }
            return symbol_table;
        }

        auto first_nonlocal_symbol_index(std::vector< output_symbol > const& symbols) const -> std::uint32_t
        {
            for (std::size_t i = 0; i < symbols.size(); ++i)
            {
                if (symbols.at(i).binding != llvm::ELF::STB_LOCAL)
                {
                    return static_cast< std::uint32_t >(i + 1);
                }
            }
            return static_cast< std::uint32_t >(symbols.size() + 1);
        }

        void add_symbol_sections()
        {
            std::vector< output_symbol > symbols = build_output_symbols();
            std::vector< std::byte > string_table = build_symbol_string_table(symbols);
            std::vector< std::byte > symbol_table = build_symbol_table(symbols);

            linked_section string_table_section;
            string_table_section.name = ".strtab";
            string_table_section.section_type = llvm::ELF::SHT_STRTAB;
            string_table_section.alignment = 1;
            string_table_section.allocated = false;
            string_table_section.synthetic = true;
            string_table_section.contents = std::move(string_table);
            string_table_section.memory_size = string_table_section.contents.size();

            std::uint32_t const string_table_section_index = static_cast< std::uint32_t >(sections.size() + 1);
            sections.push_back(std::move(string_table_section));

            linked_section symbol_table_section;
            symbol_table_section.name = ".symtab";
            symbol_table_section.section_type = llvm::ELF::SHT_SYMTAB;
            symbol_table_section.section_link = string_table_section_index;
            symbol_table_section.section_info = first_nonlocal_symbol_index(symbols);
            symbol_table_section.entry_size = machine.pointer_size_bytes() == 8 ? 24 : 16;
            symbol_table_section.alignment = machine.pointer_size_bytes() == 8 ? 8 : 4;
            symbol_table_section.allocated = false;
            symbol_table_section.synthetic = true;
            symbol_table_section.contents = std::move(symbol_table);
            symbol_table_section.memory_size = symbol_table_section.contents.size();
            sections.push_back(std::move(symbol_table_section));
        }

        auto symbol_address(llvm::object::SymbolRef const& symbol) const -> std::uint64_t
        {
            if (is_common_symbol(symbol))
            {
                if (!common_bss_section_index.has_value())
                {
                    throw quxlang::semantic_compilation_error("ELF common symbol has no allocated output section");
                }

                std::map< std::string, common_symbol_allocation >::const_iterator const allocation_iter = common_symbol_allocations.find(symbol_name(symbol));
                if (allocation_iter == common_symbol_allocations.end())
                {
                    throw quxlang::semantic_compilation_error("ELF common symbol allocation was not found");
                }

                return sections.at(*common_bss_section_index).virtual_address + allocation_iter->second.offset;
            }

            std::uint32_t const flags = take_or_throw(symbol.getFlags(), "Failed to read ELF symbol flags");
            if ((flags & llvm::object::BasicSymbolRef::SF_Undefined) != 0)
            {
                throw quxlang::semantic_compilation_error("Undefined symbol during ELF link");
            }

            llvm::object::section_iterator const section_iter = take_or_throw(symbol.getSection(), "Failed to read ELF symbol section");
            if (section_iter == object_file->section_end())
            {
                return take_or_throw(symbol.getAddress(), "Failed to read ELF absolute symbol address");
            }

            std::map< std::uint64_t, std::size_t >::const_iterator output_section_iter = output_section_indices_by_input_index.find(section_iter->getIndex());
            if (output_section_iter == output_section_indices_by_input_index.end())
            {
                throw quxlang::semantic_compilation_error("ELF link encountered a symbol in an unsupported section");
            }

            std::uint64_t const section_relative_address = take_or_throw(symbol.getAddress(), "Failed to read ELF symbol address") - section_iter->getAddress();
            linked_section const& output_section = sections.at(output_section_iter->second);
            return output_section.virtual_address + section_relative_address;
        }

        /**
         * Returns the symbol referenced by an ELF relocation.
         */
        auto relocation_target_symbol(llvm::object::RelocationRef const& relocation) const -> llvm::object::SymbolRef
        {
            llvm::object::symbol_iterator const symbol_iter = relocation.getSymbol();
            if (symbol_iter == object_file->symbol_end())
            {
                throw quxlang::semantic_compilation_error("ELF relocation has no target symbol");
            }

            return *symbol_iter;
        }

        auto relocation_target_address(llvm::object::RelocationRef const& relocation) const -> std::uint64_t
        {
            return symbol_address(relocation_target_symbol(relocation));
        }

        /**
         * Returns true when the input symbol has ELF TLS symbol type.
         */
        auto symbol_is_tls(llvm::object::SymbolRef const& symbol) const -> bool
        {
            llvm::object::ELFSymbolRef const elf_symbol(symbol);
            return elf_symbol.getELFType() == llvm::ELF::STT_TLS;
        }

        /**
         * Converts a checked unsigned 64-bit value to signed 64-bit form.
         */
        auto checked_i64(std::uint64_t value, std::string const& context) const -> std::int64_t
        {
            if (value > static_cast< std::uint64_t >(std::numeric_limits< std::int64_t >::max()))
            {
                throw quxlang::semantic_compilation_error(context);
            }
            return static_cast< std::int64_t >(value);
        }

        /**
         * Returns a TLS symbol offset relative to the start of the output TLS segment.
         */
        auto tls_symbol_offset(llvm::object::SymbolRef const& symbol) const -> std::uint64_t
        {
            if (!tls_segment_info.has_value())
            {
                throw quxlang::semantic_compilation_error("ELF TLS relocation requested without a PT_TLS segment");
            }
            if (is_common_symbol(symbol))
            {
                throw quxlang::semantic_compilation_error("ELF TLS common symbols are not supported");
            }
            if (!symbol_is_tls(symbol))
            {
                throw quxlang::semantic_compilation_error("ELF TLS relocation targeted a non-TLS symbol");
            }

            llvm::object::section_iterator const section_iter = take_or_throw(symbol.getSection(), "Failed to read ELF TLS symbol section");
            if (section_iter == object_file->section_end())
            {
                throw quxlang::semantic_compilation_error("ELF TLS relocation targeted an absolute symbol");
            }

            std::map< std::uint64_t, std::size_t >::const_iterator output_section_iter = output_section_indices_by_input_index.find(section_iter->getIndex());
            if (output_section_iter == output_section_indices_by_input_index.end())
            {
                throw quxlang::semantic_compilation_error("ELF TLS relocation targeted an unsupported section");
            }

            linked_section const& output_section = sections.at(output_section_iter->second);
            if (!output_section.tls)
            {
                throw quxlang::semantic_compilation_error("ELF TLS symbol is not in a TLS output section");
            }

            std::uint64_t const section_relative_address = take_or_throw(symbol.getAddress(), "Failed to read ELF TLS symbol address") - section_iter->getAddress();
            return output_section.virtual_address - tls_segment_info->virtual_address + section_relative_address;
        }

        /**
         * Computes a target ABI thread-pointer-relative offset for a TLS symbol.
         */
        auto tls_thread_pointer_offset(llvm::object::SymbolRef const& symbol) const -> std::int64_t
        {
            if (!tls_segment_info.has_value())
            {
                throw quxlang::semantic_compilation_error("ELF TLS relocation requested without a PT_TLS segment");
            }

            std::uint64_t const symbol_offset = tls_symbol_offset(symbol);
            std::uint64_t const alignment_mask = tls_segment_info->alignment - 1;
            switch (machine.cpu_type)
            {
            case quxlang::cpu::x86_32:
            case quxlang::cpu::x86_64:
            {
                std::uint64_t const alignment_adjustment = (0 - tls_segment_info->virtual_address - tls_segment_info->memory_size) & alignment_mask;
                return checked_i64(symbol_offset, "ELF TLS symbol offset is too large") -
                       checked_i64(tls_segment_info->memory_size, "ELF TLS segment is too large") -
                       checked_i64(alignment_adjustment, "ELF TLS alignment adjustment is too large");
            }
            case quxlang::cpu::arm_64:
            {
                std::uint64_t const thread_control_block_size = machine.pointer_size_bytes() * 2;
                std::uint64_t const alignment_adjustment = (tls_segment_info->virtual_address - thread_control_block_size) & alignment_mask;
                return checked_i64(symbol_offset, "ELF TLS symbol offset is too large") +
                       checked_i64(thread_control_block_size, "ELF TLS thread-control-block offset is too large") +
                       checked_i64(alignment_adjustment, "ELF TLS alignment adjustment is too large");
            }
            default:
                break;
            }

            throw quxlang::semantic_compilation_error("ELF TLS relocation is unsupported for this target");
        }

        /**
         * Computes the target ABI thread-pointer-relative offset for a TLS relocation.
         */
        auto relocation_target_tls_thread_pointer_offset(llvm::object::RelocationRef const& relocation) const -> std::int64_t
        {
            return tls_thread_pointer_offset(relocation_target_symbol(relocation));
        }

        auto relocation_addend(linked_section const& section, llvm::object::SectionRef const& relocation_section, llvm::object::RelocationRef const& relocation) const
            -> std::int64_t
        {
            llvm::object::ELFSectionRef const elf_relocation_section(relocation_section);
            if (elf_relocation_section.getType() == llvm::ELF::SHT_RELA)
            {
                return take_or_throw(llvm::object::ELFRelocationRef(relocation).getAddend(), "Failed to read ELF relocation addend");
            }

            std::size_t const offset = static_cast< std::size_t >(relocation.getOffset());
            std::uint64_t const relocation_type = relocation.getType();
            switch (machine.cpu_type)
            {
            case quxlang::cpu::x86_32:
                if (relocation_type == llvm::ELF::R_386_32 || relocation_type == llvm::ELF::R_386_PLT32 || relocation_type == llvm::ELF::R_386_PC32)
                {
                    return static_cast< std::int32_t >(read_u32(section.contents, offset));
                }
                break;
            default:
                break;
            }

            throw quxlang::semantic_compilation_error("Failed to derive implicit ELF relocation addend");
        }

        void collect_arm64_got_slots()
        {
            if (machine.cpu_type != quxlang::cpu::arm_64)
            {
                return;
            }

            got_slots_by_target_address.clear();
            got_section_index.reset();

            for (llvm::object::SectionRef const& generic_section : object_file->sections())
            {
                llvm::object::ELFSectionRef const elf_section(generic_section);
                if (elf_section.getType() != llvm::ELF::SHT_RELA && elf_section.getType() != llvm::ELF::SHT_REL)
                {
                    continue;
                }

                llvm::object::section_iterator const relocated_section_iter =
                    take_or_throw(generic_section.getRelocatedSection(), "Failed to identify relocated ELF section for GOT scan");
                if (relocated_section_iter == object_file->section_end())
                {
                    continue;
                }

                std::map< std::uint64_t, std::size_t >::const_iterator output_section_iter = output_section_indices_by_input_index.find(relocated_section_iter->getIndex());
                if (output_section_iter == output_section_indices_by_input_index.end())
                {
                    continue;
                }

                linked_section const& output_section = sections.at(output_section_iter->second);
                if (output_section.nobits)
                {
                    continue;
                }

                for (llvm::object::RelocationRef const& relocation : generic_section.relocations())
                {
                    std::uint64_t const relocation_type = relocation.getType();
                    if (relocation_type != llvm::ELF::R_AARCH64_ADR_GOT_PAGE && relocation_type != llvm::ELF::R_AARCH64_LD64_GOT_LO12_NC)
                    {
                        continue;
                    }

                    std::uint64_t const target_address =
                        relocation_target_address(relocation) + relocation_addend(output_section, generic_section, relocation);
                    if (got_slots_by_target_address.contains(target_address))
                    {
                        continue;
                    }

                    std::size_t const slot_index = got_slots_by_target_address.size();
                    got_slots_by_target_address[target_address] = got_slot{
                        .target_address = target_address,
                        .slot_index = slot_index,
                    };
                }
            }

            if (got_slots_by_target_address.empty())
            {
                return;
            }

            linked_section got_section;
            got_section.name = ".got";
            got_section.input_index = static_cast< std::uint64_t >(sections.size() + 1);
            got_section.alignment = 8;
            got_section.writable = false;
            got_section.executable = false;
            got_section.nobits = false;
            got_section.synthetic = true;
            got_section.memory_size = got_slots_by_target_address.size() * 8;
            got_section.contents.resize(static_cast< std::size_t >(got_section.memory_size), std::byte{0});
            got_section_index = sections.size();
            sections.push_back(std::move(got_section));

            for (std::pair< std::uint64_t const, got_slot > const& slot_entry : got_slots_by_target_address)
            {
                write_u64(sections.at(*got_section_index).contents, slot_entry.second.slot_index * 8, slot_entry.second.target_address);
            }
        }

        void refresh_arm64_got_slots()
        {
            if (!got_section_index.has_value())
            {
                return;
            }

            linked_section& got_section = sections.at(*got_section_index);
            std::fill(got_section.contents.begin(), got_section.contents.end(), std::byte{0});
            got_slots_by_target_address.clear();

            for (llvm::object::SectionRef const& generic_section : object_file->sections())
            {
                llvm::object::ELFSectionRef const elf_section(generic_section);
                if (elf_section.getType() != llvm::ELF::SHT_RELA && elf_section.getType() != llvm::ELF::SHT_REL)
                {
                    continue;
                }

                llvm::object::section_iterator const relocated_section_iter =
                    take_or_throw(generic_section.getRelocatedSection(), "Failed to identify relocated ELF section for GOT refresh");
                if (relocated_section_iter == object_file->section_end() ||
                    output_section_indices_by_input_index.find(relocated_section_iter->getIndex()) == output_section_indices_by_input_index.end())
                {
                    continue;
                }

                for (llvm::object::RelocationRef const& relocation : generic_section.relocations())
                {
                    std::uint64_t const relocation_type = relocation.getType();
                    if (relocation_type != llvm::ELF::R_AARCH64_ADR_GOT_PAGE && relocation_type != llvm::ELF::R_AARCH64_LD64_GOT_LO12_NC)
                    {
                        continue;
                    }

                    linked_section const& output_section = sections.at(output_section_indices_by_input_index.at(relocated_section_iter->getIndex()));
                    std::uint64_t const target_address =
                        relocation_target_address(relocation) + relocation_addend(output_section, generic_section, relocation);
                    if (got_slots_by_target_address.contains(target_address))
                    {
                        continue;
                    }

                    std::size_t const slot_index = got_slots_by_target_address.size();
                    if ((slot_index + 1) * 8 > got_section.contents.size())
                    {
                        throw quxlang::semantic_compilation_error("AArch64 GOT refresh changed the slot count");
                    }
                    got_slots_by_target_address[target_address] = got_slot{
                        .target_address = target_address,
                        .slot_index = slot_index,
                    };
                    write_u64(got_section.contents, slot_index * 8, target_address);
                }
            }
        }

        auto mutable_section_for_relocation(llvm::object::SectionRef const& section) -> linked_section&
        {
            std::map< std::uint64_t, std::size_t >::const_iterator output_section_iter = output_section_indices_by_input_index.find(section.getIndex());
            if (output_section_iter == output_section_indices_by_input_index.end())
            {
                throw quxlang::semantic_compilation_error("ELF relocation references an unsupported section");
            }

            return sections.at(output_section_iter->second);
        }

        auto read_u32(std::vector< std::byte > const& bytes, std::size_t offset) const -> std::uint32_t
        {
            return std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset])) | (std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset + 1])) << 8) |
                   (std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset + 2])) << 16) | (std::uint32_t(std::to_integer< std::uint8_t >(bytes[offset + 3])) << 24);
        }

        auto read_u64(std::vector< std::byte > const& bytes, std::size_t offset) const -> std::uint64_t
        {
            std::uint64_t value = 0;
            for (std::size_t i = 0; i < 8; ++i)
            {
                value |= std::uint64_t(std::to_integer< std::uint8_t >(bytes[offset + i])) << (i * 8);
            }
            return value;
        }

        void write_u16(std::vector< std::byte >& bytes, std::size_t offset, std::uint16_t value) const
        {
            bytes[offset] = static_cast< std::byte >(value & 0xff);
            bytes[offset + 1] = static_cast< std::byte >((value >> 8) & 0xff);
        }

        void write_u32(std::vector< std::byte >& bytes, std::size_t offset, std::uint32_t value) const
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                bytes[offset + i] = static_cast< std::byte >((value >> (i * 8)) & 0xff);
            }
        }

        void write_u64(std::vector< std::byte >& bytes, std::size_t offset, std::uint64_t value) const
        {
            for (std::size_t i = 0; i < 8; ++i)
            {
                bytes[offset + i] = static_cast< std::byte >((value >> (i * 8)) & 0xff);
            }
        }

        void patch_x86_64_abs64(linked_section& section, std::size_t offset, std::uint64_t target_value) const
        {
            write_u64(section.contents, offset, target_value);
        }

        /**
         * Writes a checked signed 32-bit relocation result.
         */
        void patch_signed32(linked_section& section, std::size_t offset, std::int64_t value, std::string const& context) const
        {
            if (value < std::numeric_limits< std::int32_t >::min() || value > std::numeric_limits< std::int32_t >::max())
            {
                throw quxlang::semantic_compilation_error(context);
            }

            write_u32(section.contents, offset, static_cast< std::uint32_t >(value));
        }

        void patch_i386_abs32(linked_section& section, std::size_t offset, std::uint64_t target_value) const
        {
            write_u32(section.contents, offset, static_cast< std::uint32_t >(target_value));
        }

        void patch_i386_pc32(linked_section& section, std::size_t offset, std::int64_t value) const
        {
            write_u32(section.contents, offset, static_cast< std::uint32_t >(value));
        }

        void patch_aarch64_abs64(linked_section& section, std::size_t offset, std::uint64_t target_value) const
        {
            write_u64(section.contents, offset, target_value);
        }

        void patch_aarch64_movw(linked_section& section, std::size_t offset, std::uint64_t target_value, unsigned shift) const
        {
            std::uint32_t instruction = read_u32(section.contents, offset);
            instruction &= ~(std::uint32_t(0xffff) << 5);
            instruction |= static_cast< std::uint32_t >((target_value >> shift) & 0xffff) << 5;
            write_u32(section.contents, offset, instruction);
        }

        void patch_aarch64_branch26(linked_section& section, std::size_t offset, std::uint64_t place_address, std::uint64_t target_value) const
        {
            std::int64_t const delta = static_cast< std::int64_t >(target_value) - static_cast< std::int64_t >(place_address);
            if ((delta & 0x3) != 0)
            {
                throw quxlang::semantic_compilation_error("AArch64 branch relocation target is not 4-byte aligned");
            }

            std::int64_t const immediate = delta >> 2;
            if (immediate < -(1 << 25) || immediate >= (1 << 25))
            {
                throw quxlang::semantic_compilation_error("AArch64 branch relocation is out of range");
            }

            std::uint32_t instruction = read_u32(section.contents, offset);
            instruction &= ~std::uint32_t(0x03ffffff);
            instruction |= static_cast< std::uint32_t >(immediate) & 0x03ffffff;
            write_u32(section.contents, offset, instruction);
        }

        void patch_aarch64_adr_got_page(linked_section& section, std::size_t offset, std::uint64_t place_address, std::uint64_t got_address) const
        {
            std::int64_t const page_delta = static_cast< std::int64_t >(got_address & ~std::uint64_t(0xfff)) - static_cast< std::int64_t >(place_address & ~std::uint64_t(0xfff));
            std::int64_t const immediate = page_delta >> 12;
            if (immediate < -(1 << 20) || immediate >= (1 << 20))
            {
                throw quxlang::semantic_compilation_error("AArch64 ADR_GOT_PAGE relocation is out of range");
            }

            std::uint32_t instruction = read_u32(section.contents, offset);
            instruction &= ~((std::uint32_t(0x3) << 29) | (std::uint32_t(0x7ffff) << 5));
            instruction |= (static_cast< std::uint32_t >(immediate) & 0x3) << 29;
            instruction |= ((static_cast< std::uint32_t >(immediate) >> 2) & 0x7ffff) << 5;
            write_u32(section.contents, offset, instruction);
        }

        void patch_aarch64_ld64_got_lo12(linked_section& section, std::size_t offset, std::uint64_t got_address) const
        {
            std::uint32_t instruction = read_u32(section.contents, offset);
            instruction &= ~(std::uint32_t(0xfff) << 10);
            instruction |= static_cast< std::uint32_t >((got_address & 0xfff) >> 3) << 10;
            write_u32(section.contents, offset, instruction);
        }

        /**
         * Applies an AArch64 ADD-immediate TLS relocation.
         */
        void patch_aarch64_tlsle_add_tprel(linked_section& section, std::size_t offset, std::uint64_t target_value, bool high_bits) const
        {
            if (high_bits && target_value >= (std::uint64_t{1} << 24))
            {
                throw quxlang::semantic_compilation_error("AArch64 TLSLE ADD_TPREL_HI12 relocation is out of range");
            }

            std::uint64_t const immediate = high_bits ? target_value >> 12 : target_value;
            std::uint32_t instruction = read_u32(section.contents, offset);
            instruction &= ~(std::uint32_t(0xfff) << 10);
            instruction |= static_cast< std::uint32_t >(immediate & 0xfff) << 10;
            write_u32(section.contents, offset, instruction);
        }

        auto got_slot_address(std::uint64_t target_value) const -> std::uint64_t
        {
            if (!got_section_index.has_value())
            {
                throw quxlang::semantic_compilation_error("AArch64 GOT relocation requested without a synthesized GOT section");
            }

            std::map< std::uint64_t, got_slot >::const_iterator slot_iter = got_slots_by_target_address.find(target_value);
            if (slot_iter == got_slots_by_target_address.end())
            {
                throw quxlang::semantic_compilation_error("AArch64 GOT relocation requested an unknown slot");
            }

            linked_section const& got_section = sections.at(*got_section_index);
            return got_section.virtual_address + static_cast< std::uint64_t >(slot_iter->second.slot_index) * 8;
        }

        void apply_relocations()
        {
            for (llvm::object::SectionRef const& generic_section : object_file->sections())
            {
                llvm::object::ELFSectionRef const elf_section(generic_section);
                if (elf_section.getType() != llvm::ELF::SHT_RELA && elf_section.getType() != llvm::ELF::SHT_REL)
                {
                    continue;
                }

                llvm::object::section_iterator const relocated_section_iter =
                    take_or_throw(generic_section.getRelocatedSection(), "Failed to identify relocated ELF section");
                if (relocated_section_iter == object_file->section_end())
                {
                    continue;
                }

                std::map< std::uint64_t, std::size_t >::const_iterator output_section_iter = output_section_indices_by_input_index.find(relocated_section_iter->getIndex());
                if (output_section_iter == output_section_indices_by_input_index.end())
                {
                    continue;
                }

                linked_section& section = sections.at(output_section_iter->second);
                if (section.nobits)
                {
                    continue;
                }

                for (llvm::object::RelocationRef const& relocation : generic_section.relocations())
                {
                    std::uint64_t const offset = relocation.getOffset();
                    std::int64_t const addend = relocation_addend(section, generic_section, relocation);
                    std::uint64_t const place_address = section.virtual_address + offset;
                    std::uint64_t const relocation_type = relocation.getType();

                    switch (machine.cpu_type)
                    {
                    case quxlang::cpu::x86_64:
                        if (relocation_type == llvm::ELF::R_X86_64_64)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_x86_64_abs64(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_X86_64_TPOFF32)
                        {
                            std::int64_t const target_offset = relocation_target_tls_thread_pointer_offset(relocation) + addend;
                            patch_signed32(section, static_cast< std::size_t >(offset), target_offset, "x86_64 TLS TPOFF32 relocation is out of range");
                            continue;
                        }
                        break;
                    case quxlang::cpu::x86_32:
                        if (relocation_type == llvm::ELF::R_386_32)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_i386_abs32(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_386_PLT32 || relocation_type == llvm::ELF::R_386_PC32)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_i386_pc32(section, static_cast< std::size_t >(offset), static_cast< std::int64_t >(symbol_value) + addend - static_cast< std::int64_t >(place_address));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_386_TLS_LE || relocation_type == llvm::ELF::R_386_TLS_TPOFF)
                        {
                            std::int64_t const target_offset = relocation_target_tls_thread_pointer_offset(relocation) + addend;
                            patch_signed32(section, static_cast< std::size_t >(offset), target_offset, "i386 TLS relocation is out of range");
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_386_TLS_LE_32 || relocation_type == llvm::ELF::R_386_TLS_TPOFF32)
                        {
                            std::int64_t const target_offset = -relocation_target_tls_thread_pointer_offset(relocation) + addend;
                            patch_signed32(section, static_cast< std::size_t >(offset), target_offset, "i386 TLS relocation is out of range");
                            continue;
                        }
                        break;
                    case quxlang::cpu::arm_64:
                        if (relocation_type == llvm::ELF::R_AARCH64_ABS64)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_abs64(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G0_NC)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 0);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G1_NC)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 16);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G2_NC)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 32);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G3)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 48);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_CALL26 || relocation_type == llvm::ELF::R_AARCH64_JUMP26)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_branch26(section, static_cast< std::size_t >(offset), place_address, symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_ADR_GOT_PAGE)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_adr_got_page(section, static_cast< std::size_t >(offset), place_address, got_slot_address(symbol_value + static_cast< std::uint64_t >(addend)));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_LD64_GOT_LO12_NC)
                        {
                            std::uint64_t const symbol_value = relocation_target_address(relocation);
                            patch_aarch64_ld64_got_lo12(section, static_cast< std::size_t >(offset), got_slot_address(symbol_value + static_cast< std::uint64_t >(addend)));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_TLSLE_ADD_TPREL_HI12)
                        {
                            std::int64_t const target_offset = relocation_target_tls_thread_pointer_offset(relocation) + addend;
                            if (target_offset < 0)
                            {
                                throw quxlang::semantic_compilation_error("AArch64 TLSLE ADD_TPREL relocation is negative");
                            }
                            patch_aarch64_tlsle_add_tprel(section, static_cast< std::size_t >(offset), static_cast< std::uint64_t >(target_offset), true);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_TLSLE_ADD_TPREL_LO12 ||
                            relocation_type == llvm::ELF::R_AARCH64_TLSLE_ADD_TPREL_LO12_NC)
                        {
                            std::int64_t const target_offset = relocation_target_tls_thread_pointer_offset(relocation) + addend;
                            if (target_offset < 0)
                            {
                                throw quxlang::semantic_compilation_error("AArch64 TLSLE ADD_TPREL relocation is negative");
                            }
                            patch_aarch64_tlsle_add_tprel(section, static_cast< std::size_t >(offset), static_cast< std::uint64_t >(target_offset), false);
                            continue;
                        }
                        break;
                    default:
                        break;
                    }

                    throw quxlang::semantic_compilation_error("Unsupported ELF relocation in qxc linker");
                }
            }
        }

        auto entry_address() const -> std::uint64_t
        {
            for (llvm::object::SymbolRef const& symbol : object_file->symbols())
            {
                std::string const name = take_or_throw(symbol.getName(), "Failed to read ELF symbol name").str();
                if (name != entry_symbol)
                {
                    continue;
                }

                return symbol_address(symbol);
            }

            throw quxlang::semantic_compilation_error("ELF executable entry symbol was not found: " + entry_symbol);
        }

        void copy_section_contents(std::vector< std::byte >& output_file_bytes) const
        {
            for (linked_section const& section : sections)
            {
                if (section.nobits)
                {
                    continue;
                }

                std::copy(section.contents.begin(), section.contents.end(), output_file_bytes.begin() + static_cast< std::ptrdiff_t >(section.file_offset));
            }
        }

        void write_elf_header(std::vector< std::byte >& output_file_bytes, std::uint64_t entry_point) const
        {
            if (machine.pointer_size_bytes() == 8)
            {
                output_file_bytes[0] = std::byte{0x7f};
                output_file_bytes[1] = std::byte{'E'};
                output_file_bytes[2] = std::byte{'L'};
                output_file_bytes[3] = std::byte{'F'};
                output_file_bytes[4] = std::byte{2};
                output_file_bytes[5] = std::byte{1};
                output_file_bytes[6] = std::byte{1};
                output_file_bytes[7] = std::byte{0};
                write_u16(output_file_bytes, 16, llvm::ELF::ET_EXEC);
                write_u16(output_file_bytes, 18, machine.cpu_type == quxlang::cpu::x86_64 ? llvm::ELF::EM_X86_64 : llvm::ELF::EM_AARCH64);
                write_u32(output_file_bytes, 20, 1);
                write_u64(output_file_bytes, 24, entry_point);
                write_u64(output_file_bytes, 32, 64);
                write_u64(output_file_bytes, 40, section_header_offset);
                write_u32(output_file_bytes, 48, 0);
                write_u16(output_file_bytes, 52, 64);
                write_u16(output_file_bytes, 54, 56);
                write_u16(output_file_bytes, 56, program_header_count);
                write_u16(output_file_bytes, 58, 64);
                write_u16(output_file_bytes, 60, section_header_count);
                write_u16(output_file_bytes, 62, section_header_string_table_index);
                return;
            }

            output_file_bytes[0] = std::byte{0x7f};
            output_file_bytes[1] = std::byte{'E'};
            output_file_bytes[2] = std::byte{'L'};
            output_file_bytes[3] = std::byte{'F'};
            output_file_bytes[4] = std::byte{1};
            output_file_bytes[5] = std::byte{1};
            output_file_bytes[6] = std::byte{1};
            output_file_bytes[7] = std::byte{0};
            write_u16(output_file_bytes, 16, llvm::ELF::ET_EXEC);
            write_u16(output_file_bytes, 18, llvm::ELF::EM_386);
            write_u32(output_file_bytes, 20, 1);
            write_u32(output_file_bytes, 24, static_cast< std::uint32_t >(entry_point));
            write_u32(output_file_bytes, 28, 52);
            write_u32(output_file_bytes, 32, static_cast< std::uint32_t >(section_header_offset));
            write_u32(output_file_bytes, 36, 0);
            write_u16(output_file_bytes, 40, 52);
            write_u16(output_file_bytes, 42, 32);
            write_u16(output_file_bytes, 44, program_header_count);
            write_u16(output_file_bytes, 46, 40);
            write_u16(output_file_bytes, 48, section_header_count);
            write_u16(output_file_bytes, 50, section_header_string_table_index);
        }

        /**
         * Writes all program headers for the already-computed file layout.
         */
        void write_program_header(std::vector< std::byte >& output_file_bytes) const
        {
            if (machine.pointer_size_bytes() == 8)
            {
                for (std::size_t i = 0; i < load_segments.size(); ++i)
                {
                    load_segment const& segment = load_segments.at(i);
                    std::size_t const offset = 64 + i * 56;
                    write_u32(output_file_bytes, offset, llvm::ELF::PT_LOAD);
                    write_u32(output_file_bytes, offset + 4, segment.flags);
                    write_u64(output_file_bytes, offset + 8, segment.file_offset);
                    write_u64(output_file_bytes, offset + 16, segment.virtual_address);
                    write_u64(output_file_bytes, offset + 24, segment.virtual_address);
                    write_u64(output_file_bytes, offset + 32, segment.file_size);
                    write_u64(output_file_bytes, offset + 40, segment.memory_size);
                    write_u64(output_file_bytes, offset + 48, page_alignment);
                }
                if (tls_segment_info.has_value())
                {
                    std::size_t const offset = 64 + load_segments.size() * 56;
                    write_u32(output_file_bytes, offset, llvm::ELF::PT_TLS);
                    write_u32(output_file_bytes, offset + 4, llvm::ELF::PF_R);
                    write_u64(output_file_bytes, offset + 8, tls_segment_info->file_offset);
                    write_u64(output_file_bytes, offset + 16, tls_segment_info->virtual_address);
                    write_u64(output_file_bytes, offset + 24, tls_segment_info->virtual_address);
                    write_u64(output_file_bytes, offset + 32, tls_segment_info->file_size);
                    write_u64(output_file_bytes, offset + 40, tls_segment_info->memory_size);
                    write_u64(output_file_bytes, offset + 48, tls_segment_info->alignment);
                }
                return;
            }

            for (std::size_t i = 0; i < load_segments.size(); ++i)
            {
                load_segment const& segment = load_segments.at(i);
                std::size_t const offset = 52 + i * 32;
                write_u32(output_file_bytes, offset, llvm::ELF::PT_LOAD);
                write_u32(output_file_bytes, offset + 4, static_cast< std::uint32_t >(segment.file_offset));
                write_u32(output_file_bytes, offset + 8, static_cast< std::uint32_t >(segment.virtual_address));
                write_u32(output_file_bytes, offset + 12, static_cast< std::uint32_t >(segment.virtual_address));
                write_u32(output_file_bytes, offset + 16, static_cast< std::uint32_t >(segment.file_size));
                write_u32(output_file_bytes, offset + 20, static_cast< std::uint32_t >(segment.memory_size));
                write_u32(output_file_bytes, offset + 24, segment.flags);
                write_u32(output_file_bytes, offset + 28, static_cast< std::uint32_t >(page_alignment));
            }
            if (tls_segment_info.has_value())
            {
                std::size_t const offset = 52 + load_segments.size() * 32;
                write_u32(output_file_bytes, offset, llvm::ELF::PT_TLS);
                write_u32(output_file_bytes, offset + 4, static_cast< std::uint32_t >(tls_segment_info->file_offset));
                write_u32(output_file_bytes, offset + 8, static_cast< std::uint32_t >(tls_segment_info->virtual_address));
                write_u32(output_file_bytes, offset + 12, static_cast< std::uint32_t >(tls_segment_info->virtual_address));
                write_u32(output_file_bytes, offset + 16, static_cast< std::uint32_t >(tls_segment_info->file_size));
                write_u32(output_file_bytes, offset + 20, static_cast< std::uint32_t >(tls_segment_info->memory_size));
                write_u32(output_file_bytes, offset + 24, llvm::ELF::PF_R);
                write_u32(output_file_bytes, offset + 28, static_cast< std::uint32_t >(tls_segment_info->alignment));
            }
        }

        /**
         * Writes one ELF64 section header entry at the given section-header table index.
         */
        void write_section_header_64(std::vector< std::byte >& output_file_bytes,
                                     std::size_t index,
                                     std::uint32_t name_offset,
                                     std::uint32_t type,
                                     std::uint64_t flags,
                                     std::uint64_t address,
                                     std::uint64_t offset,
                                     std::uint64_t size,
                                     std::uint64_t alignment,
                                     std::uint32_t link,
                                     std::uint32_t info,
                                     std::uint64_t entry_size) const
        {
            std::size_t const entry_offset = static_cast< std::size_t >(section_header_offset) + index * 64;
            write_u32(output_file_bytes, entry_offset, name_offset);
            write_u32(output_file_bytes, entry_offset + 4, type);
            write_u64(output_file_bytes, entry_offset + 8, flags);
            write_u64(output_file_bytes, entry_offset + 16, address);
            write_u64(output_file_bytes, entry_offset + 24, offset);
            write_u64(output_file_bytes, entry_offset + 32, size);
            write_u32(output_file_bytes, entry_offset + 40, link);
            write_u32(output_file_bytes, entry_offset + 44, info);
            write_u64(output_file_bytes, entry_offset + 48, alignment);
            write_u64(output_file_bytes, entry_offset + 56, entry_size);
        }

        /**
         * Writes one ELF32 section header entry at the given section-header table index.
         */
        void write_section_header_32(std::vector< std::byte >& output_file_bytes,
                                     std::size_t index,
                                     std::uint32_t name_offset,
                                     std::uint32_t type,
                                     std::uint64_t flags,
                                     std::uint64_t address,
                                     std::uint64_t offset,
                                     std::uint64_t size,
                                     std::uint64_t alignment,
                                     std::uint32_t link,
                                     std::uint32_t info,
                                     std::uint64_t entry_size) const
        {
            std::size_t const entry_offset = static_cast< std::size_t >(section_header_offset) + index * 40;
            write_u32(output_file_bytes, entry_offset, name_offset);
            write_u32(output_file_bytes, entry_offset + 4, type);
            write_u32(output_file_bytes, entry_offset + 8, static_cast< std::uint32_t >(flags));
            write_u32(output_file_bytes, entry_offset + 12, static_cast< std::uint32_t >(address));
            write_u32(output_file_bytes, entry_offset + 16, static_cast< std::uint32_t >(offset));
            write_u32(output_file_bytes, entry_offset + 20, static_cast< std::uint32_t >(size));
            write_u32(output_file_bytes, entry_offset + 24, link);
            write_u32(output_file_bytes, entry_offset + 28, info);
            write_u32(output_file_bytes, entry_offset + 32, static_cast< std::uint32_t >(alignment));
            write_u32(output_file_bytes, entry_offset + 36, static_cast< std::uint32_t >(entry_size));
        }

        /**
         * Writes section headers for loadable output sections and the final section-name string table.
         */
        void write_section_headers(std::vector< std::byte >& output_file_bytes) const
        {
            for (std::size_t i = 0; i < sections.size(); ++i)
            {
                linked_section const& section = sections.at(i);
                if (machine.pointer_size_bytes() == 8)
                {
                    write_section_header_64(output_file_bytes,
                                            i + 1,
                                            section.section_name_offset,
                                            section.section_type,
                                            section_flags(section),
                                            section.virtual_address,
                                            section.file_offset,
                                            section.memory_size,
                                            section.alignment,
                                            section.section_link,
                                            section.section_info,
                                            section.entry_size);
                }
                else
                {
                    write_section_header_32(output_file_bytes,
                                            i + 1,
                                            section.section_name_offset,
                                            section.section_type,
                                            section_flags(section),
                                            section.virtual_address,
                                            section.file_offset,
                                            section.memory_size,
                                            section.alignment,
                                            section.section_link,
                                            section.section_info,
                                            section.entry_size);
                }
            }

            if (machine.pointer_size_bytes() == 8)
            {
                write_section_header_64(output_file_bytes,
                                        section_header_string_table_index,
                                        section_name_table_name_offset,
                                        llvm::ELF::SHT_STRTAB,
                                        0,
                                        0,
                                        section_name_table_file_offset,
                                        section_name_table.size(),
                                        1,
                                        0,
                                        0,
                                        0);
                return;
            }

            write_section_header_32(output_file_bytes,
                                    section_header_string_table_index,
                                    section_name_table_name_offset,
                                    llvm::ELF::SHT_STRTAB,
                                    0,
                                    0,
                                    section_name_table_file_offset,
                                    section_name_table.size(),
                                    1,
                                    0,
                                    0,
                                    0);
        }

        auto build_executable_image() const -> std::vector< std::byte >
        {
            std::vector< std::byte > output_file_bytes(static_cast< std::size_t >(file_size), std::byte{0});
            copy_section_contents(output_file_bytes);
            std::copy(section_name_table.begin(), section_name_table.end(), output_file_bytes.begin() + static_cast< std::ptrdiff_t >(section_name_table_file_offset));
            std::uint64_t const entry_point = entry_address();
            write_elf_header(output_file_bytes, entry_point);
            write_program_header(output_file_bytes);
            write_section_headers(output_file_bytes);
            return output_file_bytes;
        }
    };
} // namespace quxlang::detail

auto quxlang::elf_linker::link_linux_executable(quxlang::machine_target_info const& machine,
                                                std::vector< std::byte > const& object_file,
                                                std::string const& entry_symbol,
                                                quxlang::elf_link_options const& options) const
    -> std::vector< std::byte >
{
    quxlang::detail::elf_link_session session(machine, object_file, entry_symbol, options);
    return session.link();
}
