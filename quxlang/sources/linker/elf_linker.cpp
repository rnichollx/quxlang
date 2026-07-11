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
#include <set>
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

    /**
     * dynamic_import_layout stores the output indices assigned to one runtime-loaded procedure.
     */
    struct dynamic_import_layout
    {
        quxlang::elf_dynamic_import import;
        std::string soname;
        std::uint32_t symbol_name_offset = 0;
        std::uint16_t version_index = llvm::ELF::VER_NDX_GLOBAL;
        std::size_t dynamic_symbol_index = 0;
        std::size_t procedure_linkage_table_index = 0;
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
            add_dynamic_link_sections();
            layout_sections();
            std::set< std::string > const undefined_symbols = collect_undefined_relocation_symbols();
            if (!undefined_symbols.empty())
            {
                throw quxlang::semantic_compilation_error(undefined_symbols_message(undefined_symbols));
            }
            collect_arm64_got_slots();
            if (got_section_index.has_value())
            {
                layout_sections();
                refresh_arm64_got_slots();
            }
            populate_dynamic_link_sections();
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
        std::vector< dynamic_import_layout > dynamic_imports;
        std::map< std::string, std::size_t > dynamic_import_indices_by_relocation_symbol;
        std::vector< std::string > needed_sonames;
        std::map< std::string, std::uint32_t > dynamic_string_offsets;
        std::map< std::string, std::vector< std::pair< std::string, std::uint16_t > > > versions_by_soname;
        std::optional< std::size_t > interpreter_section_index;
        std::optional< std::size_t > dynamic_string_section_index;
        std::optional< std::size_t > dynamic_symbol_section_index;
        std::optional< std::size_t > symbol_version_section_index;
        std::optional< std::size_t > version_need_section_index;
        std::optional< std::size_t > procedure_relocation_section_index;
        std::optional< std::size_t > procedure_linkage_table_section_index;
        std::optional< std::size_t > procedure_got_section_index;
        std::optional< std::size_t > dynamic_section_index;
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

        /**
         * Returns the user-facing spelling for one raw object-file symbol name.
         */
        auto display_symbol_name(std::string const& raw_symbol_name) const -> std::string
        {
            std::map< std::string, std::string >::const_iterator const display_name = options.symbol_display_names.find(raw_symbol_name);
            if (display_name == options.symbol_display_names.end())
            {
                return raw_symbol_name;
            }
            if (display_name->second == raw_symbol_name)
            {
                return raw_symbol_name;
            }
            return display_name->second + " (" + raw_symbol_name + ")";
        }

        /**
         * Returns true when an ELF symbol reference is undefined.
         */
        auto symbol_is_undefined(llvm::object::SymbolRef const& symbol) const -> bool
        {
            std::uint32_t const flags = take_or_throw(symbol.getFlags(), "Failed to read ELF symbol flags");
            return (flags & llvm::object::BasicSymbolRef::SF_Undefined) != 0;
        }

        /**
         * Formats the undefined-symbol linker diagnostic.
         */
        auto undefined_symbols_message(std::set< std::string > const& undefined_symbols) const -> std::string
        {
            std::string message = undefined_symbols.size() == 1 ? "Undefined symbol during ELF link: " : "Undefined symbols during ELF link: ";
            bool first = true;
            for (std::string const& raw_symbol_name : undefined_symbols)
            {
                if (!first)
                {
                    message += ", ";
                }
                message += display_symbol_name(raw_symbol_name);
                first = false;
            }
            return message;
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

        /**
         * Returns the runtime loader pathname encoded for a self-contained glibc target.
         */
        auto glibc_interpreter_path() const -> std::string
        {
            switch (machine.cpu_type)
            {
            case quxlang::cpu::x86_64:
                return "/lib64/ld-linux-x86-64.so.2";
            case quxlang::cpu::x86_32:
                return "/lib/ld-linux.so.2";
            case quxlang::cpu::arm_64:
                return "/lib/ld-linux-aarch64.so.1";
            default:
                break;
            }
            throw quxlang::semantic_compilation_error("glibc dynamic linking is not supported for this CPU target");
        }

        /**
         * Resolves one declaration-level library name without reading a host sysroot.
         */
        auto dynamic_library_soname(std::string const& library_name) const -> std::string
        {
            if (machine.environment_type == quxlang::environment::glibc && library_name == "glibc")
            {
                return "libc.so.6";
            }
            throw quxlang::semantic_compilation_error("ELF dynamic library is not supported for this target environment: " + library_name);
        }

        /**
         * Adds the allocated sections whose sizes are determined by dynamic import declarations.
         */
        void add_dynamic_link_sections()
        {
            if (options.dynamic_imports.empty())
            {
                return;
            }
            if (machine.environment_type != quxlang::environment::glibc)
            {
                throw quxlang::semantic_compilation_error("ELF dynamic imports require a supported hosted target environment");
            }
            if (machine.cpu_type != quxlang::cpu::x86_64)
            {
                throw quxlang::semantic_compilation_error("ELF dynamic imports are currently supported for Linux x86-64 targets");
            }

            dynamic_imports.clear();
            dynamic_import_indices_by_relocation_symbol.clear();
            needed_sonames.clear();
            dynamic_string_offsets.clear();
            versions_by_soname.clear();

            std::vector< std::byte > dynamic_strings(1, std::byte{0});
            auto add_dynamic_string = [&](std::string const& value) -> std::uint32_t
            {
                std::map< std::string, std::uint32_t >::const_iterator const existing = dynamic_string_offsets.find(value);
                if (existing != dynamic_string_offsets.end())
                {
                    return existing->second;
                }
                if (dynamic_strings.size() > std::numeric_limits< std::uint32_t >::max())
                {
                    throw quxlang::semantic_compilation_error("ELF dynamic string table is too large");
                }
                std::uint32_t const offset = static_cast< std::uint32_t >(dynamic_strings.size());
                for (char const character : value)
                {
                    dynamic_strings.push_back(static_cast< std::byte >(character));
                }
                dynamic_strings.push_back(std::byte{0});
                dynamic_string_offsets.emplace(value, offset);
                return offset;
            };

            std::map< std::pair< std::string, std::string >, std::uint16_t > version_indices;
            std::uint32_t next_version_index = 2;
            for (quxlang::elf_dynamic_import const& import : options.dynamic_imports)
            {
                if (import.relocation_symbol_name.empty() || import.symbol_name.empty() || import.library_name.empty())
                {
                    throw quxlang::semantic_compilation_error("ELF dynamic import metadata is incomplete");
                }
                if (dynamic_import_indices_by_relocation_symbol.contains(import.relocation_symbol_name))
                {
                    continue;
                }

                std::string const soname = dynamic_library_soname(import.library_name);
                if (std::find(needed_sonames.begin(), needed_sonames.end(), soname) == needed_sonames.end())
                {
                    needed_sonames.push_back(soname);
                    add_dynamic_string(soname);
                }

                std::uint16_t version_index = llvm::ELF::VER_NDX_GLOBAL;
                if (!import.version.empty())
                {
                    std::pair< std::string, std::string > const version_key(soname, import.version);
                    std::map< std::pair< std::string, std::string >, std::uint16_t >::const_iterator const existing_version = version_indices.find(version_key);
                    if (existing_version != version_indices.end())
                    {
                        version_index = existing_version->second;
                    }
                    else
                    {
                        if (next_version_index > std::numeric_limits< std::uint16_t >::max())
                        {
                            throw quxlang::semantic_compilation_error("ELF dynamic symbol version table is too large");
                        }
                        version_index = static_cast< std::uint16_t >(next_version_index++);
                        version_indices.emplace(version_key, version_index);
                        versions_by_soname[soname].push_back(std::make_pair(import.version, version_index));
                        add_dynamic_string(import.version);
                    }
                }

                std::size_t const import_index = dynamic_imports.size();
                dynamic_import_indices_by_relocation_symbol.emplace(import.relocation_symbol_name, import_index);
                dynamic_imports.push_back(dynamic_import_layout{
                    .import = import,
                    .soname = soname,
                    .symbol_name_offset = add_dynamic_string(import.symbol_name),
                    .version_index = version_index,
                    .dynamic_symbol_index = import_index + 1,
                    .procedure_linkage_table_index = import_index + 1,
                });
            }

            auto append_section = [&](std::string name,
                                      std::uint32_t type,
                                      std::uint64_t alignment,
                                      bool writable,
                                      bool executable,
                                      std::uint64_t entry_size,
                                      std::vector< std::byte > contents) -> std::size_t
            {
                linked_section section;
                section.name = std::move(name);
                section.input_index = static_cast< std::uint64_t >(sections.size() + 1);
                section.section_type = type;
                section.entry_size = entry_size;
                section.alignment = alignment;
                section.allocated = true;
                section.writable = writable;
                section.executable = executable;
                section.synthetic = true;
                section.contents = std::move(contents);
                section.memory_size = section.contents.size();
                std::size_t const index = sections.size();
                sections.push_back(std::move(section));
                return index;
            };

            std::string const interpreter = glibc_interpreter_path();
            std::vector< std::byte > interpreter_contents;
            for (char const character : interpreter)
            {
                interpreter_contents.push_back(static_cast< std::byte >(character));
            }
            interpreter_contents.push_back(std::byte{0});

            interpreter_section_index = append_section(".interp", llvm::ELF::SHT_PROGBITS, 1, false, false, 0, std::move(interpreter_contents));
            dynamic_string_section_index = append_section(".dynstr", llvm::ELF::SHT_STRTAB, 1, false, false, 0, std::move(dynamic_strings));
            dynamic_symbol_section_index = append_section(".dynsym", llvm::ELF::SHT_DYNSYM, 8, false, false, 24, std::vector< std::byte >((dynamic_imports.size() + 1) * 24));
            symbol_version_section_index = append_section(".gnu.version", llvm::ELF::SHT_GNU_versym, 2, false, false, 2, std::vector< std::byte >((dynamic_imports.size() + 1) * 2));

            std::size_t version_need_size = 0;
            for (std::pair< std::string const, std::vector< std::pair< std::string, std::uint16_t > > > const& library_versions : versions_by_soname)
            {
                version_need_size += 16 + library_versions.second.size() * 16;
            }
            if (version_need_size != 0)
            {
                version_need_section_index = append_section(".gnu.version_r", llvm::ELF::SHT_GNU_verneed, 8, false, false, 0, std::vector< std::byte >(version_need_size));
            }

            procedure_relocation_section_index = append_section(".rela.plt", llvm::ELF::SHT_RELA, 8, false, false, 24, std::vector< std::byte >(dynamic_imports.size() * 24));
            procedure_linkage_table_section_index = append_section(".plt", llvm::ELF::SHT_PROGBITS, 16, false, true, 16, std::vector< std::byte >((dynamic_imports.size() + 1) * 16));
            procedure_got_section_index = append_section(".got.plt", llvm::ELF::SHT_PROGBITS, 8, true, false, 8, std::vector< std::byte >((dynamic_imports.size() + 3) * 8));

            std::size_t const dynamic_entry_count = needed_sonames.size() + 11 + (version_need_section_index.has_value() ? 2 : 0);
            dynamic_section_index = append_section(".dynamic", llvm::ELF::SHT_DYNAMIC, 8, true, false, 16, std::vector< std::byte >(dynamic_entry_count * 16));

            sections.at(*dynamic_symbol_section_index).section_link = static_cast< std::uint32_t >(*dynamic_string_section_index + 1);
            sections.at(*dynamic_symbol_section_index).section_info = 1;
            sections.at(*symbol_version_section_index).section_link = static_cast< std::uint32_t >(*dynamic_symbol_section_index + 1);
            if (version_need_section_index.has_value())
            {
                sections.at(*version_need_section_index).section_link = static_cast< std::uint32_t >(*dynamic_string_section_index + 1);
                sections.at(*version_need_section_index).section_info = static_cast< std::uint32_t >(versions_by_soname.size());
            }
            sections.at(*procedure_relocation_section_index).section_link = static_cast< std::uint32_t >(*dynamic_symbol_section_index + 1);
            sections.at(*procedure_relocation_section_index).section_info = static_cast< std::uint32_t >(*procedure_got_section_index + 1);
            sections.at(*dynamic_section_index).section_link = static_cast< std::uint32_t >(*dynamic_string_section_index + 1);
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
            std::uint16_t const dynamic_program_header_count = dynamic_imports.empty() ? 0 : 2;
            program_header_count = static_cast< std::uint16_t >(1 + count_loadable_section_runs() + (has_tls_sections() ? 1 : 0) + dynamic_program_header_count);
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
                throw quxlang::semantic_compilation_error(undefined_symbols_message({symbol_name(symbol)}));
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

        /**
         * Returns every undefined symbol targeted by a relocation the linker would apply.
         */
        auto collect_undefined_relocation_symbols() const -> std::set< std::string >
        {
            std::set< std::string > result;
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

                linked_section const& section = sections.at(output_section_iter->second);
                if (section.nobits)
                {
                    continue;
                }

                for (llvm::object::RelocationRef const& relocation : generic_section.relocations())
                {
                    llvm::object::SymbolRef const target_symbol = relocation_target_symbol(relocation);
                    if (symbol_is_undefined(target_symbol))
                    {
                        std::string const undefined_symbol_name = symbol_name(target_symbol);
                        if (!dynamic_import_indices_by_relocation_symbol.contains(undefined_symbol_name))
                        {
                            result.insert(undefined_symbol_name);
                        }
                    }
                }
            }
            return result;
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

        /**
         * Computes the ELF hash recorded in one GNU version-need auxiliary entry.
         */
        auto gnu_symbol_version_hash(std::string const& version) const -> std::uint32_t
        {
            std::uint32_t hash = 0;
            for (unsigned char const character : version)
            {
                hash = (hash << 4) + character;
                std::uint32_t const high = hash & 0xf0000000U;
                if (high != 0)
                {
                    hash ^= high >> 24;
                }
                hash &= ~high;
            }
            return hash;
        }

        /**
         * Returns the procedure-linkage-table address assigned to one undefined import symbol.
         */
        auto dynamic_import_plt_address(std::string const& relocation_symbol_name) const -> std::uint64_t
        {
            std::map< std::string, std::size_t >::const_iterator const found = dynamic_import_indices_by_relocation_symbol.find(relocation_symbol_name);
            if (found == dynamic_import_indices_by_relocation_symbol.end() || !procedure_linkage_table_section_index.has_value())
            {
                throw quxlang::semantic_compilation_error("ELF relocation does not name a configured dynamic import: " + relocation_symbol_name);
            }
            return sections.at(*procedure_linkage_table_section_index).virtual_address + dynamic_imports.at(found->second).procedure_linkage_table_index * 16;
        }

        /**
         * Populates address-dependent ELF dynamic-loader structures after section layout.
         */
        void populate_dynamic_link_sections()
        {
            if (dynamic_imports.empty())
            {
                return;
            }

            linked_section& dynamic_symbols = sections.at(*dynamic_symbol_section_index);
            linked_section& symbol_versions = sections.at(*symbol_version_section_index);
            linked_section& procedure_relocations = sections.at(*procedure_relocation_section_index);
            linked_section& procedure_linkage_table = sections.at(*procedure_linkage_table_section_index);
            linked_section& procedure_got = sections.at(*procedure_got_section_index);
            linked_section& dynamic_section = sections.at(*dynamic_section_index);
            linked_section const& dynamic_strings = sections.at(*dynamic_string_section_index);

            write_u16(symbol_versions.contents, 0, llvm::ELF::VER_NDX_LOCAL);
            write_u64(procedure_got.contents, 0, dynamic_section.virtual_address);

            procedure_linkage_table.contents.at(0) = std::byte{0xff};
            procedure_linkage_table.contents.at(1) = std::byte{0x35};
            patch_signed32(procedure_linkage_table,
                           2,
                           static_cast< std::int64_t >(procedure_got.virtual_address + 8) - static_cast< std::int64_t >(procedure_linkage_table.virtual_address + 6),
                           "x86-64 PLT resolver GOT displacement is out of range");
            procedure_linkage_table.contents.at(6) = std::byte{0xff};
            procedure_linkage_table.contents.at(7) = std::byte{0x25};
            patch_signed32(procedure_linkage_table,
                           8,
                           static_cast< std::int64_t >(procedure_got.virtual_address + 16) - static_cast< std::int64_t >(procedure_linkage_table.virtual_address + 12),
                           "x86-64 PLT resolver jump displacement is out of range");
            procedure_linkage_table.contents.at(12) = std::byte{0x0f};
            procedure_linkage_table.contents.at(13) = std::byte{0x1f};
            procedure_linkage_table.contents.at(14) = std::byte{0x40};
            procedure_linkage_table.contents.at(15) = std::byte{0x00};

            for (std::size_t i = 0; i < dynamic_imports.size(); ++i)
            {
                dynamic_import_layout const& import = dynamic_imports.at(i);
                std::size_t const symbol_offset = import.dynamic_symbol_index * 24;
                write_u32(dynamic_symbols.contents, symbol_offset, import.symbol_name_offset);
                std::uint8_t const binding = import.import.optional ? llvm::ELF::STB_WEAK : llvm::ELF::STB_GLOBAL;
                dynamic_symbols.contents.at(symbol_offset + 4) = static_cast< std::byte >((binding << 4) | llvm::ELF::STT_FUNC);
                write_u16(dynamic_symbols.contents, symbol_offset + 6, llvm::ELF::SHN_UNDEF);
                write_u16(symbol_versions.contents, import.dynamic_symbol_index * 2, import.version_index);

                std::uint64_t const got_slot_address = procedure_got.virtual_address + (i + 3) * 8;
                std::size_t const relocation_offset = i * 24;
                write_u64(procedure_relocations.contents, relocation_offset, got_slot_address);
                write_u64(procedure_relocations.contents,
                          relocation_offset + 8,
                          (static_cast< std::uint64_t >(import.dynamic_symbol_index) << 32) | llvm::ELF::R_X86_64_JUMP_SLOT);
                write_u64(procedure_relocations.contents, relocation_offset + 16, 0);

                std::size_t const plt_offset = import.procedure_linkage_table_index * 16;
                std::uint64_t const plt_address = procedure_linkage_table.virtual_address + plt_offset;
                procedure_linkage_table.contents.at(plt_offset) = std::byte{0xff};
                procedure_linkage_table.contents.at(plt_offset + 1) = std::byte{0x25};
                patch_signed32(procedure_linkage_table,
                               plt_offset + 2,
                               static_cast< std::int64_t >(got_slot_address) - static_cast< std::int64_t >(plt_address + 6),
                               "x86-64 PLT import GOT displacement is out of range");
                procedure_linkage_table.contents.at(plt_offset + 6) = std::byte{0x68};
                write_u32(procedure_linkage_table.contents, plt_offset + 7, static_cast< std::uint32_t >(i));
                procedure_linkage_table.contents.at(plt_offset + 11) = std::byte{0xe9};
                patch_signed32(procedure_linkage_table,
                               plt_offset + 12,
                               static_cast< std::int64_t >(procedure_linkage_table.virtual_address) - static_cast< std::int64_t >(plt_address + 16),
                               "x86-64 PLT resolver displacement is out of range");
                write_u64(procedure_got.contents, (i + 3) * 8, plt_address + 6);
            }

            if (version_need_section_index.has_value())
            {
                linked_section& version_need = sections.at(*version_need_section_index);
                std::size_t record_offset = 0;
                std::size_t library_index = 0;
                for (std::pair< std::string const, std::vector< std::pair< std::string, std::uint16_t > > > const& library_versions : versions_by_soname)
                {
                    std::size_t const record_size = 16 + library_versions.second.size() * 16;
                    write_u16(version_need.contents, record_offset, llvm::ELF::VER_NEED_CURRENT);
                    write_u16(version_need.contents, record_offset + 2, static_cast< std::uint16_t >(library_versions.second.size()));
                    write_u32(version_need.contents, record_offset + 4, dynamic_string_offsets.at(library_versions.first));
                    write_u32(version_need.contents, record_offset + 8, 16);
                    write_u32(version_need.contents, record_offset + 12, library_index + 1 == versions_by_soname.size() ? 0 : static_cast< std::uint32_t >(record_size));

                    for (std::size_t version_i = 0; version_i < library_versions.second.size(); ++version_i)
                    {
                        std::pair< std::string, std::uint16_t > const& version = library_versions.second.at(version_i);
                        std::size_t const auxiliary_offset = record_offset + 16 + version_i * 16;
                        write_u32(version_need.contents, auxiliary_offset, gnu_symbol_version_hash(version.first));
                        write_u16(version_need.contents, auxiliary_offset + 4, 0);
                        write_u16(version_need.contents, auxiliary_offset + 6, version.second);
                        write_u32(version_need.contents, auxiliary_offset + 8, dynamic_string_offsets.at(version.first));
                        write_u32(version_need.contents, auxiliary_offset + 12, version_i + 1 == library_versions.second.size() ? 0 : 16);
                    }
                    record_offset += record_size;
                    ++library_index;
                }
            }

            std::size_t dynamic_offset = 0;
            auto write_dynamic_entry = [&](std::int64_t tag, std::uint64_t value)
            {
                write_u64(dynamic_section.contents, dynamic_offset, static_cast< std::uint64_t >(tag));
                write_u64(dynamic_section.contents, dynamic_offset + 8, value);
                dynamic_offset += 16;
            };
            for (std::string const& soname : needed_sonames)
            {
                write_dynamic_entry(llvm::ELF::DT_NEEDED, dynamic_string_offsets.at(soname));
            }
            write_dynamic_entry(llvm::ELF::DT_STRTAB, dynamic_strings.virtual_address);
            write_dynamic_entry(llvm::ELF::DT_STRSZ, dynamic_strings.contents.size());
            write_dynamic_entry(llvm::ELF::DT_SYMTAB, dynamic_symbols.virtual_address);
            write_dynamic_entry(llvm::ELF::DT_SYMENT, 24);
            write_dynamic_entry(llvm::ELF::DT_PLTGOT, procedure_got.virtual_address);
            write_dynamic_entry(llvm::ELF::DT_PLTRELSZ, procedure_relocations.contents.size());
            write_dynamic_entry(llvm::ELF::DT_PLTREL, llvm::ELF::DT_RELA);
            write_dynamic_entry(llvm::ELF::DT_JMPREL, procedure_relocations.virtual_address);
            write_dynamic_entry(llvm::ELF::DT_RELAENT, 24);
            write_dynamic_entry(llvm::ELF::DT_VERSYM, symbol_versions.virtual_address);
            if (version_need_section_index.has_value())
            {
                write_dynamic_entry(llvm::ELF::DT_VERNEED, sections.at(*version_need_section_index).virtual_address);
                write_dynamic_entry(llvm::ELF::DT_VERNEEDNUM, versions_by_soname.size());
            }
            write_dynamic_entry(llvm::ELF::DT_NULL, 0);
            if (dynamic_offset != dynamic_section.contents.size())
            {
                throw quxlang::semantic_compilation_error("ELF dynamic table size does not match its entries");
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
                            llvm::object::SymbolRef const target_symbol = relocation_target_symbol(relocation);
                            std::uint64_t const symbol_value = symbol_is_undefined(target_symbol)
                                                                   ? dynamic_import_plt_address(symbol_name(target_symbol))
                                                                   : symbol_address(target_symbol);
                            patch_x86_64_abs64(section, static_cast< std::size_t >(offset), symbol_value + static_cast< std::uint64_t >(addend));
                            continue;
                        }
                        if (relocation_type == llvm::ELF::R_X86_64_PLT32 || relocation_type == llvm::ELF::R_X86_64_PC32)
                        {
                            llvm::object::SymbolRef const target_symbol = relocation_target_symbol(relocation);
                            std::uint64_t symbol_value = 0;
                            if (symbol_is_undefined(target_symbol))
                            {
                                symbol_value = dynamic_import_plt_address(symbol_name(target_symbol));
                            }
                            else
                            {
                                symbol_value = symbol_address(target_symbol);
                            }
                            patch_signed32(section,
                                           static_cast< std::size_t >(offset),
                                           static_cast< std::int64_t >(symbol_value) + addend - static_cast< std::int64_t >(place_address),
                                           "x86-64 PC-relative relocation is out of range");
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
                std::size_t program_index = 0;
                if (interpreter_section_index.has_value())
                {
                    linked_section const& interpreter = sections.at(*interpreter_section_index);
                    std::size_t const offset = 64 + program_index++ * 56;
                    write_u32(output_file_bytes, offset, llvm::ELF::PT_INTERP);
                    write_u32(output_file_bytes, offset + 4, llvm::ELF::PF_R);
                    write_u64(output_file_bytes, offset + 8, interpreter.file_offset);
                    write_u64(output_file_bytes, offset + 16, interpreter.virtual_address);
                    write_u64(output_file_bytes, offset + 24, interpreter.virtual_address);
                    write_u64(output_file_bytes, offset + 32, interpreter.contents.size());
                    write_u64(output_file_bytes, offset + 40, interpreter.contents.size());
                    write_u64(output_file_bytes, offset + 48, 1);
                }
                for (load_segment const& segment : load_segments)
                {
                    std::size_t const offset = 64 + program_index++ * 56;
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
                    std::size_t const offset = 64 + program_index++ * 56;
                    write_u32(output_file_bytes, offset, llvm::ELF::PT_TLS);
                    write_u32(output_file_bytes, offset + 4, llvm::ELF::PF_R);
                    write_u64(output_file_bytes, offset + 8, tls_segment_info->file_offset);
                    write_u64(output_file_bytes, offset + 16, tls_segment_info->virtual_address);
                    write_u64(output_file_bytes, offset + 24, tls_segment_info->virtual_address);
                    write_u64(output_file_bytes, offset + 32, tls_segment_info->file_size);
                    write_u64(output_file_bytes, offset + 40, tls_segment_info->memory_size);
                    write_u64(output_file_bytes, offset + 48, tls_segment_info->alignment);
                }
                if (dynamic_section_index.has_value())
                {
                    linked_section const& dynamic_section = sections.at(*dynamic_section_index);
                    std::size_t const offset = 64 + program_index++ * 56;
                    write_u32(output_file_bytes, offset, llvm::ELF::PT_DYNAMIC);
                    write_u32(output_file_bytes, offset + 4, llvm::ELF::PF_R | llvm::ELF::PF_W);
                    write_u64(output_file_bytes, offset + 8, dynamic_section.file_offset);
                    write_u64(output_file_bytes, offset + 16, dynamic_section.virtual_address);
                    write_u64(output_file_bytes, offset + 24, dynamic_section.virtual_address);
                    write_u64(output_file_bytes, offset + 32, dynamic_section.contents.size());
                    write_u64(output_file_bytes, offset + 40, dynamic_section.contents.size());
                    write_u64(output_file_bytes, offset + 48, 8);
                }
                if (program_index != program_header_count)
                {
                    throw quxlang::semantic_compilation_error("ELF program-header count does not match its entries");
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
