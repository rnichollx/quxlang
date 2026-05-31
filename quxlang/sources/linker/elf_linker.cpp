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
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace quxlang::detail
{
    namespace llvm = ::llvm;

    struct linked_section
    {
        std::string name;
        std::uint64_t input_index = 0;
        std::uint64_t alignment = 1;
        bool writable = false;
        bool executable = false;
        bool nobits = false;
        bool synthetic = false;
        std::vector< std::byte > contents;
        std::uint64_t memory_size = 0;
        std::uint64_t file_offset = 0;
        std::uint64_t virtual_address = 0;
    };

    struct got_slot
    {
        std::uint64_t target_address = 0;
        std::size_t slot_index = 0;
    };

    class elf_link_session
    {
    public:
        elf_link_session(quxlang::machine_target_info const& machine_info, std::vector< std::byte > const& object_file_bytes, std::string entry_symbol_name)
            : machine(machine_info),
              object_bytes(object_file_bytes),
              entry_symbol(std::move(entry_symbol_name))
        {
        }

        auto link() -> std::vector< std::byte >
        {
            validate_machine();
            open_object();
            collect_sections();
            layout_sections();
            collect_arm64_got_slots();
            if (got_section_index.has_value())
            {
                layout_sections();
            }
            apply_relocations();
            return build_executable_image();
        }

    private:
        quxlang::machine_target_info machine;
        std::vector< std::byte > const& object_bytes;
        std::string entry_symbol;
        std::unique_ptr< llvm::MemoryBuffer > object_buffer;
        std::unique_ptr< llvm::object::ObjectFile > object_file;
        std::vector< linked_section > sections;
        std::map< std::uint64_t, std::size_t > output_section_indices_by_input_index;
        std::map< std::uint64_t, got_slot > got_slots_by_target_address;
        std::optional< std::size_t > got_section_index;
        std::uint64_t executable_base_address = 0;
        std::uint64_t page_alignment = 0x1000;
        std::uint64_t file_size = 0;
        std::uint64_t memory_size = 0;

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
                output_section.alignment = std::max< std::uint64_t >(1, generic_section.getAlignment().value());
                output_section.writable = (section.getFlags() & llvm::ELF::SHF_WRITE) != 0;
                output_section.executable = (section.getFlags() & llvm::ELF::SHF_EXECINSTR) != 0;
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

        auto align_up(std::uint64_t value, std::uint64_t alignment) const -> std::uint64_t
        {
            if (alignment <= 1)
            {
                return value;
            }
            std::uint64_t const mask = alignment - 1;
            return (value + mask) & ~mask;
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
            std::uint64_t const header_end = elf_header_size + program_header_size;
            std::uint64_t file_cursor = align_up(header_end, page_alignment);
            std::uint64_t memory_cursor = file_cursor;

            for (linked_section& section : sections)
            {
                if (section.nobits)
                {
                    continue;
                }

                file_cursor = align_up(file_cursor, section.alignment);
                memory_cursor = align_up(memory_cursor, section.alignment);
                section.file_offset = file_cursor;
                section.virtual_address = executable_base_address + memory_cursor;
                file_cursor += section.contents.size();
                memory_cursor += section.memory_size;
            }

            file_size = file_cursor;

            for (linked_section& section : sections)
            {
                if (!section.nobits)
                {
                    continue;
                }

                memory_cursor = align_up(memory_cursor, section.alignment);
                section.file_offset = file_size;
                section.virtual_address = executable_base_address + memory_cursor;
                memory_cursor += section.memory_size;
            }

            memory_size = memory_cursor;
        }

        auto symbol_address(llvm::object::SymbolRef const& symbol) const -> std::uint64_t
        {
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

        auto relocation_target_address(llvm::object::RelocationRef const& relocation) const -> std::uint64_t
        {
            llvm::object::symbol_iterator const symbol_iter = relocation.getSymbol();
            if (symbol_iter == object_file->symbol_end())
            {
                throw quxlang::semantic_compilation_error("ELF relocation has no target symbol");
            }

            return symbol_address(*symbol_iter);
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
                    std::uint64_t const symbol_value = relocation_target_address(relocation);
                    std::uint64_t const place_address = section.virtual_address + offset;
                    std::uint64_t const relocation_type = relocation.getType();

                    switch (machine.cpu_type)
                    {
                    case quxlang::cpu::x86_64:
                        if (relocation_type == llvm::ELF::R_X86_64_64)
                        {
                            patch_x86_64_abs64(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        break;
                    case quxlang::cpu::x86_32:
                        if (relocation_type == llvm::ELF::R_386_32)
                        {
                            patch_i386_abs32(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_386_PLT32 || relocation_type == llvm::ELF::R_386_PC32)
                        {
                            patch_i386_pc32(section, static_cast< std::size_t >(offset), static_cast< std::int64_t >(symbol_value) + addend - static_cast< std::int64_t >(place_address));
                            continue;
                        }
                        break;
                    case quxlang::cpu::arm_64:
                        if (relocation_type == llvm::ELF::R_AARCH64_ABS64)
                        {
                            patch_aarch64_abs64(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G0_NC)
                        {
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 0);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G1_NC)
                        {
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 16);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G2_NC)
                        {
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 32);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_MOVW_UABS_G3)
                        {
                            patch_aarch64_movw(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend), 48);
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_CALL26 || relocation_type == llvm::ELF::R_AARCH64_JUMP26)
                        {
                            patch_aarch64_branch26(section, static_cast< std::size_t >(offset), place_address, symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_ADR_GOT_PAGE)
                        {
                            patch_aarch64_adr_got_page(section, static_cast< std::size_t >(offset), place_address, got_slot_address(symbol_value + static_cast< std::uint64_t >(addend)));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_AARCH64_LD64_GOT_LO12_NC)
                        {
                            patch_aarch64_ld64_got_lo12(section, static_cast< std::size_t >(offset), got_slot_address(symbol_value + static_cast< std::uint64_t >(addend)));
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
                write_u64(output_file_bytes, 40, 0);
                write_u32(output_file_bytes, 48, 0);
                write_u16(output_file_bytes, 52, 64);
                write_u16(output_file_bytes, 54, 56);
                write_u16(output_file_bytes, 56, 1);
                write_u16(output_file_bytes, 58, 0);
                write_u16(output_file_bytes, 60, 0);
                write_u16(output_file_bytes, 62, 0);
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
            write_u32(output_file_bytes, 32, 0);
            write_u32(output_file_bytes, 36, 0);
            write_u16(output_file_bytes, 40, 52);
            write_u16(output_file_bytes, 42, 32);
            write_u16(output_file_bytes, 44, 1);
            write_u16(output_file_bytes, 46, 0);
            write_u16(output_file_bytes, 48, 0);
            write_u16(output_file_bytes, 50, 0);
        }

        void write_program_header(std::vector< std::byte >& output_file_bytes) const
        {
            if (machine.pointer_size_bytes() == 8)
            {
                write_u32(output_file_bytes, 64, llvm::ELF::PT_LOAD);
                write_u32(output_file_bytes, 68, llvm::ELF::PF_R | llvm::ELF::PF_W | llvm::ELF::PF_X);
                write_u64(output_file_bytes, 72, 0);
                write_u64(output_file_bytes, 80, executable_base_address);
                write_u64(output_file_bytes, 88, executable_base_address);
                write_u64(output_file_bytes, 96, file_size);
                write_u64(output_file_bytes, 104, memory_size);
                write_u64(output_file_bytes, 112, page_alignment);
                return;
            }

            write_u32(output_file_bytes, 52, llvm::ELF::PT_LOAD);
            write_u32(output_file_bytes, 56, 0);
            write_u32(output_file_bytes, 60, static_cast< std::uint32_t >(executable_base_address));
            write_u32(output_file_bytes, 64, static_cast< std::uint32_t >(executable_base_address));
            write_u32(output_file_bytes, 68, static_cast< std::uint32_t >(file_size));
            write_u32(output_file_bytes, 72, static_cast< std::uint32_t >(memory_size));
            write_u32(output_file_bytes, 76, llvm::ELF::PF_R | llvm::ELF::PF_W | llvm::ELF::PF_X);
            write_u32(output_file_bytes, 80, static_cast< std::uint32_t >(page_alignment));
        }

        auto build_executable_image() const -> std::vector< std::byte >
        {
            std::vector< std::byte > output_file_bytes(static_cast< std::size_t >(file_size), std::byte{0});
            copy_section_contents(output_file_bytes);
            std::uint64_t const entry_point = entry_address();
            write_elf_header(output_file_bytes, entry_point);
            write_program_header(output_file_bytes);
            return output_file_bytes;
        }
    };
} // namespace quxlang::detail

auto quxlang::elf_linker::link_linux_executable(quxlang::machine_target_info const& machine, std::vector< std::byte > const& object_file, std::string const& entry_symbol) const
    -> std::vector< std::byte >
{
    quxlang::detail::elf_link_session session(machine, object_file, entry_symbol);
    return session.link();
}
