// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/linker/macho_linker.hpp>

#include <quxlang/data/compilation_result.hpp>

#include "macho_linker_internal.hpp"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SHA256.h>
#include <rpnx/macros.hpp>

#include <algorithm>
#include <array>
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

    struct macho_input_section_id
    {
        std::size_t object_index = 0;
        std::uint32_t section_ordinal = 0;

        RPNX_MEMBER_METADATA(macho_input_section_id, object_index, section_ordinal);
    };

    struct macho_input_object
    {
        std::unique_ptr< llvm::MemoryBuffer > buffer;
        std::unique_ptr< llvm::object::ObjectFile > object_file;
    };

    struct macho_input_symbol
    {
        std::string name;
        std::uint8_t type = 0;
        std::uint8_t section_ordinal = 0;
        std::uint64_t value = 0;
    };

    struct macho_resolved_symbol
    {
        std::optional< std::pair< std::size_t, std::size_t > > definition;
        bool definition_is_weak = false;
        bool common = false;
        std::uint64_t common_offset = 0;
        std::uint64_t common_size = 0;
        std::uint64_t common_alignment = 1;
    };

    struct macho_section_placement
    {
        std::size_t output_section_index = 0;
        std::uint64_t output_offset = 0;
    };

    struct macho_input_section
    {
        macho_input_section_id id;
        std::string segment_name;
        std::string section_name;
        std::uint64_t input_address = 0;
        std::uint64_t memory_size = 0;
        std::uint64_t alignment = 1;
        std::uint32_t flags = 0;
        bool zerofill = false;
        std::vector< std::byte > contents;
    };

    struct macho_output_section
    {
        std::string segment_name;
        std::string section_name;
        std::uint64_t alignment = 1;
        std::uint32_t flags = 0;
        bool zerofill = false;
        bool synthetic = false;
        std::vector< std::byte > contents;
        std::uint64_t memory_size = 0;
        std::uint64_t file_offset = 0;
        std::uint64_t virtual_address = 0;
    };

    struct macho_got_slot
    {
        std::size_t slot_index = 0;
    };

    struct macho_dynamic_import_layout
    {
        quxlang::macho_dynamic_import import;
        std::size_t stub_index = 0;
    };

    struct macho_symbol_reference
    {
        std::string global_name;
        std::size_t object_index = 0;
        std::size_t symbol_index = 0;

        RPNX_MEMBER_METADATA(macho_symbol_reference, global_name, object_index, symbol_index);
    };

    struct macho_segment_layout
    {
        std::uint64_t file_offset = 0;
        std::uint64_t file_size = 0;
        std::uint64_t virtual_address = 0;
        std::uint64_t memory_size = 0;
    };

    class macho_link_session
    {
    public:
        /** Creates one isolated link session for the supplied object images. */
        macho_link_session(quxlang::machine_target_info machine_info,
                           std::vector< std::vector< std::byte > > const& object_file_bytes,
                           std::string entry_symbol_name,
                           quxlang::macho_link_options link_options) :
            machine(machine_info),
            input_object_bytes(object_file_bytes),
            entry_symbol(std::move(entry_symbol_name)),
            options(std::move(link_options))
        {
        }

        /** Validates, lays out, relocates, signs, and returns the final executable image. */
        auto link() -> std::vector< std::byte >
        {
            validate_machine();
            open_objects();
            collect_symbols();
            collect_sections();
            allocate_common_symbols();
            collect_dynamic_imports();
            collect_got_slots();
            allocate_dynamic_import_stubs();
            layout_sections();
            populate_got();
            populate_dynamic_import_stubs();
            apply_relocations();
            finalize_linkedit_layout();
            return build_executable_image();
        }

    private:
        quxlang::machine_target_info machine;
        std::vector< std::vector< std::byte > > const& input_object_bytes;
        std::string entry_symbol;
        quxlang::macho_link_options options;
        std::vector< macho_input_object > input_objects;
        std::vector< std::vector< macho_input_symbol > > input_symbols;
        std::map< std::string, macho_resolved_symbol > global_symbols;
        std::map< macho_input_section_id, macho_section_placement > section_placements;
        std::map< macho_input_section_id, std::uint64_t > section_input_addresses;
        std::vector< macho_output_section > output_sections;
        std::map< std::pair< std::string, std::string >, std::size_t > output_section_indices;
        std::map< macho_symbol_reference, macho_got_slot > got_slots;
        std::map< std::string, macho_dynamic_import_layout > dynamic_imports;
        std::optional< std::size_t > got_output_section_index;
        std::optional< std::size_t > import_stubs_output_section_index;
        std::optional< std::size_t > common_output_section_index;
        macho_segment_layout text_segment;
        macho_segment_layout data_segment;
        macho_segment_layout linkedit_segment;
        std::vector< std::uint64_t > rebase_addresses;
        std::vector< std::byte > rebase_data;
        std::uint64_t rebase_data_offset = 0;
        std::vector< std::byte > bind_data;
        std::uint64_t bind_data_offset = 0;
        std::uint64_t code_signature_offset = 0;
        std::uint64_t code_signature_size = 0;
        std::uint64_t entry_address = 0;

        /** Rounds value upward to a validated power-of-two alignment. */
        static auto align_up(std::uint64_t value, std::uint64_t alignment) -> std::uint64_t
        {
            if (alignment == 0 || (alignment & (alignment - 1)) != 0)
            {
                throw quxlang::semantic_compilation_error("Mach-O linker alignment is not a power of two");
            }
            if (value > std::numeric_limits< std::uint64_t >::max() - (alignment - 1))
            {
                throw quxlang::semantic_compilation_error("Mach-O linker layout exceeds the address range");
            }
            return (value + alignment - 1) & ~(alignment - 1);
        }

        /** Removes the Mach-O C symbol prefix used by LLVM-generated objects. */
        static auto canonical_symbol_name(std::string name) -> std::string
        {
            if (!name.empty() && name.front() == '_')
            {
                name.erase(name.begin());
            }
            return name;
        }

        /** Converts an LLVM error into compiler diagnostic text. */
        static auto llvm_error_text(llvm::Error error) -> std::string
        {
            std::string result;
            llvm::handleAllErrors(std::move(error),
                                  [&](llvm::ErrorInfoBase const& info)
                                  {
                                      result = info.message();
                                  });
            return result;
        }

        /** Returns the target architecture's Mach-O segment page size. */
        auto page_size() const -> std::uint64_t
        {
            return machine.cpu_type == quxlang::cpu::arm_64 ? 0x4000 : 0x1000;
        }

        /** Maps the configured Quxlang CPU to its Mach-O CPU type. */
        auto expected_cpu_type() const -> std::uint32_t
        {
            switch (machine.cpu_type)
            {
            case quxlang::cpu::arm_64:
                return llvm::MachO::CPU_TYPE_ARM64;
            case quxlang::cpu::x86_64:
                return llvm::MachO::CPU_TYPE_X86_64;
            default:
                break;
            }
            throw quxlang::semantic_compilation_error("Mach-O linker does not support this target CPU");
        }

        /** Maps the configured Quxlang CPU to its baseline Mach-O CPU subtype. */
        auto expected_cpu_subtype() const -> std::uint32_t
        {
            switch (machine.cpu_type)
            {
            case quxlang::cpu::arm_64:
                return llvm::MachO::CPU_SUBTYPE_ARM64_ALL;
            case quxlang::cpu::x86_64:
                return llvm::MachO::CPU_SUBTYPE_X86_64_ALL;
            default:
                break;
            }
            throw quxlang::semantic_compilation_error("Mach-O linker does not support this target CPU");
        }

        /** Rejects target configurations that this Mach-O linker cannot represent. */
        void validate_machine() const
        {
            if (machine.os_type != quxlang::os::macos || machine.binary_type != quxlang::binary::macho)
            {
                throw quxlang::semantic_compilation_error("Mach-O linker requires a macOS Mach-O target");
            }
            (void)expected_cpu_type();
            if (machine.pointer_size_bytes() != 8)
            {
                throw quxlang::semantic_compilation_error("Mach-O linker currently requires a 64-bit target");
            }
            if (options.signature_identifier.empty())
            {
                throw quxlang::semantic_compilation_error("Mach-O code-signature identifier cannot be empty");
            }
        }

        /** Returns one input as its already-validated Mach-O object type. */
        auto object(std::size_t object_index) const -> llvm::object::MachOObjectFile const&
        {
            return *llvm::cast< llvm::object::MachOObjectFile >(input_objects.at(object_index).object_file.get());
        }

        /** Parses and validates every in-memory relocatable input object. */
        void open_objects()
        {
            if (input_object_bytes.empty())
            {
                throw quxlang::semantic_compilation_error(
                    "Mach-O linker requires at least one relocatable input object");
            }

            input_objects.reserve(input_object_bytes.size());
            for (std::size_t object_index = 0; object_index < input_object_bytes.size(); ++object_index)
            {
                std::vector< std::byte > const& bytes = input_object_bytes.at(object_index);
                std::string object_text(reinterpret_cast< char const* >(bytes.data()), bytes.size());
                macho_input_object input_object;
                input_object.buffer = llvm::MemoryBuffer::getMemBufferCopy(
                    object_text, "qxc-macho-link-input-" + std::to_string(object_index));
                llvm::Expected< std::unique_ptr< llvm::object::ObjectFile > > object_or_error =
                    llvm::object::ObjectFile::createObjectFile(input_object.buffer->getMemBufferRef());
                if (!object_or_error)
                {
                    throw quxlang::semantic_compilation_error("Failed to open relocatable Mach-O input " +
                                                              std::to_string(object_index) + ": " +
                                                              llvm_error_text(object_or_error.takeError()));
                }
                if (!llvm::isa< llvm::object::MachOObjectFile >(object_or_error->get()))
                {
                    throw quxlang::semantic_compilation_error("Mach-O linker input " + std::to_string(object_index) +
                                                              " is not a Mach-O object file");
                }

                llvm::object::MachOObjectFile const& macho =
                    *llvm::cast< llvm::object::MachOObjectFile >(object_or_error->get());
                llvm::MachO::mach_header_64 header = macho.getHeader64();
                if (header.filetype != llvm::MachO::MH_OBJECT || header.cputype != expected_cpu_type() ||
                    !macho.is64Bit() || !macho.isLittleEndian())
                {
                    throw quxlang::semantic_compilation_error("Mach-O linker input " + std::to_string(object_index) +
                                                              " does not match the target machine");
                }

                input_object.object_file = std::move(*object_or_error);
                input_objects.push_back(std::move(input_object));
            }
        }

        /** Reads input symbol tables and selects global definitions. */
        void collect_symbols()
        {
            input_symbols.resize(input_objects.size());
            for (std::size_t object_index = 0; object_index < input_objects.size(); ++object_index)
            {
                llvm::object::MachOObjectFile const& macho = object(object_index);
                std::vector< macho_input_symbol >& symbols = input_symbols.at(object_index);
                for (llvm::object::SymbolRef symbol : macho.symbols())
                {
                    llvm::Expected< llvm::StringRef > name_or_error = symbol.getName();
                    if (!name_or_error)
                    {
                        throw quxlang::semantic_compilation_error("Failed to read Mach-O symbol name: " +
                                                                  llvm_error_text(name_or_error.takeError()));
                    }
                    llvm::MachO::nlist_64 entry = macho.getSymbol64TableEntry(symbol.getRawDataRefImpl());
                    macho_input_symbol parsed{
                        .name = name_or_error->str(),
                        .type = entry.n_type,
                        .section_ordinal = entry.n_sect,
                        .value = entry.n_value,
                    };
                    symbols.push_back(parsed);

                    if ((entry.n_type & llvm::MachO::N_STAB) != 0 || (entry.n_type & llvm::MachO::N_EXT) == 0 ||
                        parsed.name.empty())
                    {
                        continue;
                    }

                    std::string name = canonical_symbol_name(parsed.name);
                    std::uint8_t symbol_type = entry.n_type & llvm::MachO::N_TYPE;
                    macho_resolved_symbol& resolved = global_symbols[name];
                    if (symbol_type == llvm::MachO::N_UNDF && entry.n_value != 0)
                    {
                        std::uint8_t alignment_power = llvm::MachO::GET_COMM_ALIGN(entry.n_desc);
                        std::uint64_t alignment = std::uint64_t{1} << alignment_power;
                        if (!resolved.definition.has_value())
                        {
                            resolved.common = true;
                            resolved.common_size = std::max(resolved.common_size, entry.n_value);
                            resolved.common_alignment = std::max(resolved.common_alignment, alignment);
                        }
                        continue;
                    }
                    if (symbol_type == llvm::MachO::N_UNDF)
                    {
                        continue;
                    }
                    if (symbol_type != llvm::MachO::N_SECT && symbol_type != llvm::MachO::N_ABS)
                    {
                        throw quxlang::semantic_compilation_error("Unsupported Mach-O symbol type for " + parsed.name);
                    }

                    bool is_weak = (entry.n_desc & llvm::MachO::N_WEAK_DEF) != 0;
                    if (resolved.definition.has_value())
                    {
                        if (!resolved.definition_is_weak && !is_weak)
                        {
                            throw quxlang::semantic_compilation_error("Duplicate strong Mach-O symbol definition: " +
                                                                      name);
                        }
                        if (!resolved.definition_is_weak)
                        {
                            continue;
                        }
                        if (is_weak)
                        {
                            continue;
                        }
                    }

                    resolved.definition = std::make_pair(object_index, symbols.size() - 1);
                    resolved.definition_is_weak = is_weak;
                    resolved.common = false;
                }
            }
        }

        /** Reads the fixed-width Mach-O section name without trailing null bytes. */
        static auto section_name(llvm::object::MachOObjectFile const& macho, llvm::object::SectionRef section)
            -> std::string
        {
            llvm::ArrayRef< char > raw_name_bytes = macho.getSectionRawName(section.getRawDataRefImpl());
            llvm::StringRef raw_name(raw_name_bytes.data(), raw_name_bytes.size());
            return raw_name.split('\0').first.str();
        }

        /** Reads the fixed-width Mach-O segment name without trailing null bytes. */
        static auto segment_name(llvm::object::MachOObjectFile const& macho, llvm::object::SectionRef section)
            -> std::string
        {
            llvm::ArrayRef< char > raw_name_bytes = macho.getSectionRawFinalSegmentName(section.getRawDataRefImpl());
            llvm::StringRef raw_name(raw_name_bytes.data(), raw_name_bytes.size());
            return raw_name.split('\0').first.str();
        }

        /** Decodes one non-scattered relocation into its architecture-neutral fields. */
        static auto plain_relocation(llvm::object::MachOObjectFile const& macho, llvm::object::RelocationRef relocation)
            -> llvm::MachO::relocation_info
        {
            llvm::MachO::any_relocation_info encoded = macho.getRelocation(relocation.getRawDataRefImpl());
            if (macho.isRelocationScattered(encoded))
            {
                throw quxlang::semantic_compilation_error("Scattered Mach-O relocations are not supported");
            }
            llvm::MachO::relocation_info result{};
            result.r_address = static_cast< std::int32_t >(macho.getAnyRelocationAddress(encoded));
            result.r_symbolnum = macho.getPlainRelocationSymbolNum(encoded);
            result.r_pcrel = macho.getAnyRelocationPCRel(encoded);
            result.r_length = macho.getAnyRelocationLength(encoded);
            result.r_extern = macho.getPlainRelocationExternal(encoded);
            result.r_type = macho.getAnyRelocationType(encoded);
            return result;
        }

        /** Reports whether the section occupies memory without file contents. */
        static auto section_is_zerofill(std::uint32_t flags) -> bool
        {
            std::uint32_t section_type = flags & llvm::MachO::SECTION_TYPE;
            return section_type == llvm::MachO::S_ZEROFILL || section_type == llvm::MachO::S_GB_ZEROFILL;
        }

        /** Reports whether the section uses Mach-O thread-local storage semantics. */
        static auto section_is_thread_local(std::uint32_t flags) -> bool
        {
            std::uint32_t section_type = flags & llvm::MachO::SECTION_TYPE;
            return section_type == llvm::MachO::S_THREAD_LOCAL_REGULAR ||
                   section_type == llvm::MachO::S_THREAD_LOCAL_ZEROFILL ||
                   section_type == llvm::MachO::S_THREAD_LOCAL_VARIABLES ||
                   section_type == llvm::MachO::S_THREAD_LOCAL_VARIABLE_POINTERS ||
                   section_type == llvm::MachO::S_THREAD_LOCAL_INIT_FUNCTION_POINTERS;
        }

        /** Reports whether a metadata-only input section is omitted from the executable. */
        static auto section_is_discarded(std::string const& segment, std::string const& name, std::uint32_t flags)
            -> bool
        {
            return (flags & llvm::MachO::S_ATTR_DEBUG) != 0 || segment == "__DWARF" || segment == "__LD" ||
                   segment == "__LLVM" || name == "__compact_unwind" || name == "__eh_frame" ||
                   name == "__llvm_addrsig";
        }

        /** Selects or creates the compatible output section for one input contribution. */
        auto select_output_section(std::string segment, std::string name, std::uint64_t alignment, std::uint32_t flags,
                                   bool zerofill) -> std::size_t
        {
            if (segment != "__TEXT")
            {
                segment = "__DATA";
            }
            if (zerofill)
            {
                flags = (flags & ~llvm::MachO::SECTION_TYPE) | llvm::MachO::S_ZEROFILL;
            }

            std::pair< std::string, std::string > key{segment, name};
            std::map< std::pair< std::string, std::string >, std::size_t >::iterator found =
                output_section_indices.find(key);
            if (found != output_section_indices.end())
            {
                macho_output_section& output = output_sections.at(found->second);
                if (output.zerofill != zerofill ||
                    (output.flags & llvm::MachO::SECTION_TYPE) != (flags & llvm::MachO::SECTION_TYPE))
                {
                    throw quxlang::semantic_compilation_error("Incompatible Mach-O input sections share the name " +
                                                              segment + "," + name);
                }
                output.alignment = std::max(output.alignment, alignment);
                output.flags |= flags & ~llvm::MachO::SECTION_TYPE;
                return found->second;
            }

            std::size_t index = output_sections.size();
            output_sections.push_back(macho_output_section{
                .segment_name = std::move(segment),
                .section_name = std::move(name),
                .alignment = alignment,
                .flags = flags,
                .zerofill = zerofill,
            });
            output_section_indices.emplace(std::move(key), index);
            return index;
        }

        /** Merges allocated input sections and records each contribution's placement. */
        void collect_sections()
        {
            for (std::size_t object_index = 0; object_index < input_objects.size(); ++object_index)
            {
                llvm::object::MachOObjectFile const& macho = object(object_index);
                for (llvm::object::SectionRef section : macho.sections())
                {
                    llvm::MachO::section_64 header = macho.getSection64(section.getRawDataRefImpl());
                    std::string segment = segment_name(macho, section);
                    std::string name = section_name(macho, section);
                    std::uint32_t ordinal = macho.getSectionID(section) + 1;
                    if (section_is_discarded(segment, name, header.flags))
                    {
                        continue;
                    }
                    if (section_is_thread_local(header.flags))
                    {
                        throw quxlang::semantic_compilation_error(
                            "Mach-O thread-local sections are not supported yet: " + segment + "," + name);
                    }
                    if (name.size() > 16)
                    {
                        throw quxlang::semantic_compilation_error("Mach-O section name exceeds 16 bytes: " + name);
                    }

                    bool zerofill = section_is_zerofill(header.flags);
                    std::uint64_t alignment = std::uint64_t{1} << header.align;
                    macho_input_section input{
                        .id = macho_input_section_id{.object_index = object_index, .section_ordinal = ordinal},
                        .segment_name = segment,
                        .section_name = name,
                        .input_address = header.addr,
                        .memory_size = header.size,
                        .alignment = alignment,
                        .flags = header.flags,
                        .zerofill = zerofill,
                    };
                    if (!zerofill)
                    {
                        llvm::Expected< llvm::ArrayRef< std::uint8_t > > contents_or_error =
                            macho.getSectionContents(section.getRawDataRefImpl());
                        if (!contents_or_error)
                        {
                            throw quxlang::semantic_compilation_error("Failed to read Mach-O section contents: " +
                                                                      llvm_error_text(contents_or_error.takeError()));
                        }
                        input.contents.reserve(contents_or_error->size());
                        for (std::uint8_t byte : *contents_or_error)
                        {
                            input.contents.push_back(static_cast< std::byte >(byte));
                        }
                    }

                    std::size_t output_index = select_output_section(segment, name, alignment, header.flags, zerofill);
                    macho_output_section& output = output_sections.at(output_index);
                    std::uint64_t output_offset = align_up(output.memory_size, alignment);
                    output.memory_size = output_offset + header.size;
                    if (!zerofill)
                    {
                        output.contents.resize(static_cast< std::size_t >(output_offset), std::byte{});
                        output.contents.insert(output.contents.end(), input.contents.begin(), input.contents.end());
                    }
                    section_placements.emplace(input.id, macho_section_placement{
                                                             .output_section_index = output_index,
                                                             .output_offset = output_offset,
                                                         });
                    section_input_addresses.emplace(input.id, header.addr);
                }
            }
        }

        /** Allocates unresolved common symbols in a synthetic zero-fill section. */
        void allocate_common_symbols()
        {
            std::uint64_t maximum_alignment = 1;
            bool have_common = false;
            for (std::pair< std::string const, macho_resolved_symbol >& entry : global_symbols)
            {
                macho_resolved_symbol& symbol = entry.second;
                if (symbol.definition.has_value() || !symbol.common)
                {
                    continue;
                }
                have_common = true;
                maximum_alignment = std::max(maximum_alignment, symbol.common_alignment);
            }
            if (!have_common)
            {
                return;
            }

            std::size_t output_index =
                select_output_section("__DATA", "__common", maximum_alignment, llvm::MachO::S_ZEROFILL, true);
            common_output_section_index = output_index;
            macho_output_section& common = output_sections.at(output_index);
            common.synthetic = true;
            for (std::pair< std::string const, macho_resolved_symbol >& entry : global_symbols)
            {
                macho_resolved_symbol& symbol = entry.second;
                if (symbol.definition.has_value() || !symbol.common)
                {
                    continue;
                }
                symbol.common_offset = align_up(common.memory_size, symbol.common_alignment);
                common.memory_size = symbol.common_offset + symbol.common_size;
            }
        }

        /** Validates configured imports and allocates their deterministic GOT identities. */
        void collect_dynamic_imports()
        {
            for (quxlang::macho_dynamic_import const& import : options.dynamic_imports)
            {
                if (import.relocation_symbol_name.empty() || import.symbol_name.empty())
                {
                    throw quxlang::semantic_compilation_error("Mach-O dynamic import symbol names cannot be empty");
                }
                if (import.library_name != "libsystem")
                {
                    throw quxlang::semantic_compilation_error("Unsupported Mach-O dynamic import library: " +
                                                              import.library_name);
                }
                if (dynamic_imports.contains(import.relocation_symbol_name))
                {
                    throw quxlang::semantic_compilation_error("Duplicate Mach-O dynamic import relocation symbol: " +
                                                              import.relocation_symbol_name);
                }
                std::map< std::string, macho_resolved_symbol >::const_iterator resolved =
                    global_symbols.find(import.relocation_symbol_name);
                if (resolved != global_symbols.end() &&
                    (resolved->second.definition.has_value() || resolved->second.common))
                {
                    throw quxlang::semantic_compilation_error(
                        "Mach-O dynamic import conflicts with a defined symbol: " + import.relocation_symbol_name);
                }
                dynamic_imports.emplace(import.relocation_symbol_name, macho_dynamic_import_layout{.import = import});
            }

            for (std::pair< std::string const, macho_dynamic_import_layout >& entry : dynamic_imports)
            {
                macho_symbol_reference reference{.global_name = entry.first};
                entry.second.stub_index = got_slots.size();
                got_slots.emplace(std::move(reference), macho_got_slot{.slot_index = got_slots.size()});
            }
        }

        /** Reports whether a target relocation addresses a global-offset-table slot. */
        static auto relocation_uses_got(quxlang::cpu target_cpu, std::uint8_t relocation_type) -> bool
        {
            if (target_cpu == quxlang::cpu::arm_64)
            {
                return relocation_type == llvm::MachO::ARM64_RELOC_GOT_LOAD_PAGE21 ||
                       relocation_type == llvm::MachO::ARM64_RELOC_GOT_LOAD_PAGEOFF12 ||
                       relocation_type == llvm::MachO::ARM64_RELOC_POINTER_TO_GOT;
            }
            if (target_cpu == quxlang::cpu::x86_64)
            {
                return relocation_type == llvm::MachO::X86_64_RELOC_GOT_LOAD ||
                       relocation_type == llvm::MachO::X86_64_RELOC_GOT;
            }
            return false;
        }

        /** Identifies the global or object-local symbol selected by an external relocation. */
        auto relocation_symbol_reference(std::size_t object_index, llvm::MachO::relocation_info relocation) const
            -> macho_symbol_reference
        {
            if (!relocation.r_extern || relocation.r_symbolnum >= input_symbols.at(object_index).size())
            {
                throw quxlang::semantic_compilation_error("Mach-O relocation has an invalid external symbol index");
            }
            macho_input_symbol const& symbol = input_symbols.at(object_index).at(relocation.r_symbolnum);
            if ((symbol.type & llvm::MachO::N_EXT) != 0)
            {
                return macho_symbol_reference{.global_name = canonical_symbol_name(symbol.name)};
            }
            return macho_symbol_reference{
                .object_index = object_index,
                .symbol_index = relocation.r_symbolnum,
            };
        }

        /** Allocates one synthetic GOT slot for every referenced symbol identity. */
        void collect_got_slots()
        {
            for (std::size_t object_index = 0; object_index < input_objects.size(); ++object_index)
            {
                llvm::object::MachOObjectFile const& macho = object(object_index);
                for (llvm::object::SectionRef section : macho.sections())
                {
                    std::uint32_t ordinal = macho.getSectionID(section) + 1;
                    macho_input_section_id id{.object_index = object_index, .section_ordinal = ordinal};
                    if (!section_placements.contains(id))
                    {
                        continue;
                    }
                    for (llvm::object::RelocationRef relocation : section.relocations())
                    {
                        llvm::MachO::relocation_info raw = plain_relocation(macho, relocation);
                        if (!relocation_uses_got(machine.cpu_type, raw.r_type))
                        {
                            continue;
                        }
                        macho_symbol_reference reference = relocation_symbol_reference(object_index, raw);
                        if (!got_slots.contains(reference))
                        {
                            got_slots.emplace(
                                std::move(reference),
                                macho_got_slot{
                                    .slot_index = got_slots.size(),
                                });
                        }
                    }
                }
            }

            if (got_slots.empty())
            {
                return;
            }
            std::size_t output_index = select_output_section("__DATA", "__got", 8, llvm::MachO::S_REGULAR, false);
            got_output_section_index = output_index;
            macho_output_section& got = output_sections.at(output_index);
            got.synthetic = true;
            got.memory_size = got_slots.size() * 8;
            got.contents.assign(static_cast< std::size_t >(got.memory_size), std::byte{});
        }

        /** Returns the architecture-specific byte size of one dynamic-import stub. */
        auto dynamic_import_stub_size() const -> std::uint64_t
        {
            if (machine.cpu_type == quxlang::cpu::arm_64)
            {
                return 12;
            }
            if (machine.cpu_type == quxlang::cpu::x86_64)
            {
                return 6;
            }
            throw quxlang::semantic_compilation_error("Mach-O dynamic imports do not support this target CPU");
        }

        /** Allocates executable jump stubs for all configured dynamic imports. */
        void allocate_dynamic_import_stubs()
        {
            if (dynamic_imports.empty())
            {
                return;
            }
            std::uint64_t alignment = machine.cpu_type == quxlang::cpu::arm_64 ? 4 : 1;
            std::size_t output_index = select_output_section(
                "__TEXT", "__qxc_stubs", alignment,
                static_cast< std::uint32_t >(llvm::MachO::S_REGULAR) |
                    static_cast< std::uint32_t >(llvm::MachO::S_ATTR_PURE_INSTRUCTIONS) |
                    static_cast< std::uint32_t >(llvm::MachO::S_ATTR_SOME_INSTRUCTIONS),
                false);
            import_stubs_output_section_index = output_index;
            macho_output_section& stubs = output_sections.at(output_index);
            stubs.synthetic = true;
            stubs.memory_size = dynamic_imports.size() * dynamic_import_stub_size();
            stubs.contents.assign(static_cast< std::size_t >(stubs.memory_size), std::byte{});
        }

        /** Counts final sections assigned to the executable text segment. */
        auto text_section_count() const -> std::size_t
        {
            return static_cast< std::size_t >(std::count_if(output_sections.begin(), output_sections.end(),
                                                            [](macho_output_section const& section)
                                                            {
                                                                return section.segment_name == "__TEXT";
                                                            }));
        }

        /** Counts final sections assigned to the writable data segment. */
        auto data_section_count() const -> std::size_t
        {
            return output_sections.size() - text_section_count();
        }

        /** Computes the exact encoded size of every Mach-O load command. */
        auto load_commands_size() const -> std::uint64_t
        {
            std::uint64_t pagezero_size = sizeof(llvm::MachO::segment_command_64);
            std::uint64_t text_size =
                sizeof(llvm::MachO::segment_command_64) + text_section_count() * sizeof(llvm::MachO::section_64);
            std::uint64_t data_size =
                sizeof(llvm::MachO::segment_command_64) + data_section_count() * sizeof(llvm::MachO::section_64);
            std::uint64_t linkedit_size = sizeof(llvm::MachO::segment_command_64);
            return pagezero_size + text_size + data_size + linkedit_size + sizeof(llvm::MachO::dyld_info_command) + 32 +
                   sizeof(llvm::MachO::build_version_command) + sizeof(llvm::MachO::entry_point_command) + 56 +
                   sizeof(llvm::MachO::linkedit_data_command);
        }

        /** Assigns file offsets and preferred virtual addresses to final sections. */
        void layout_sections()
        {
            std::uint64_t base_address = 0x100000000;
            std::uint64_t cursor = sizeof(llvm::MachO::mach_header_64) + load_commands_size();
            for (macho_output_section& section : output_sections)
            {
                if (section.segment_name != "__TEXT")
                {
                    continue;
                }
                cursor = align_up(cursor, section.alignment);
                section.file_offset = cursor;
                section.virtual_address = base_address + cursor;
                cursor += section.contents.size();
            }
            text_segment = macho_segment_layout{
                .file_offset = 0,
                .file_size = align_up(cursor, page_size()),
                .virtual_address = base_address,
                .memory_size = align_up(cursor, page_size()),
            };

            cursor = text_segment.file_size;
            data_segment.file_offset = cursor;
            data_segment.virtual_address = base_address + cursor;
            for (macho_output_section& section : output_sections)
            {
                if (section.segment_name != "__DATA" || section.zerofill)
                {
                    continue;
                }
                cursor = align_up(cursor, section.alignment);
                section.file_offset = cursor;
                section.virtual_address = base_address + cursor;
                cursor += section.contents.size();
            }
            data_segment.file_size = cursor - data_segment.file_offset;
            std::uint64_t data_memory_cursor = cursor;
            for (macho_output_section& section : output_sections)
            {
                if (section.segment_name != "__DATA" || !section.zerofill)
                {
                    continue;
                }
                data_memory_cursor = align_up(data_memory_cursor, section.alignment);
                section.file_offset = 0;
                section.virtual_address = base_address + data_memory_cursor;
                data_memory_cursor += section.memory_size;
            }
            data_segment.memory_size = align_up(data_memory_cursor - data_segment.file_offset, page_size());

            rebase_data_offset = align_up(cursor, page_size());

            entry_address = resolved_symbol_address(entry_symbol);
            if (entry_address < text_segment.virtual_address ||
                entry_address >= text_segment.virtual_address + text_segment.memory_size)
            {
                throw quxlang::semantic_compilation_error("Mach-O entry symbol is not in the executable segment: " +
                                                          entry_symbol);
            }
        }

        /** Returns the final virtual address of one input section contribution. */
        auto section_base_address(macho_input_section_id id) const -> std::uint64_t
        {
            std::map< macho_input_section_id, macho_section_placement >::const_iterator placement =
                section_placements.find(id);
            if (placement == section_placements.end())
            {
                throw quxlang::semantic_compilation_error("Mach-O relocation or symbol refers to a discarded section");
            }
            macho_output_section const& output = output_sections.at(placement->second.output_section_index);
            return output.virtual_address + placement->second.output_offset;
        }

        /** Returns an input section's original object-file address. */
        auto input_section_address(macho_input_section_id id) const -> std::uint64_t
        {
            std::map< macho_input_section_id, std::uint64_t >::const_iterator found = section_input_addresses.find(id);
            if (found == section_input_addresses.end())
            {
                throw quxlang::semantic_compilation_error("Mach-O symbol refers to an unavailable input section");
            }
            return found->second;
        }

        /** Resolves one input symbol table record to its final virtual address. */
        auto input_symbol_address(std::size_t object_index, std::size_t symbol_index) const -> std::uint64_t
        {
            macho_input_symbol const& symbol = input_symbols.at(object_index).at(symbol_index);
            std::uint8_t symbol_type = symbol.type & llvm::MachO::N_TYPE;
            if (symbol_type == llvm::MachO::N_ABS)
            {
                return symbol.value;
            }
            if (symbol_type != llvm::MachO::N_SECT || symbol.section_ordinal == 0)
            {
                throw quxlang::semantic_compilation_error("Mach-O symbol is not defined: " + symbol.name);
            }
            macho_input_section_id id{
                .object_index = object_index,
                .section_ordinal = symbol.section_ordinal,
            };
            std::uint64_t original_section_address = input_section_address(id);
            if (symbol.value < original_section_address)
            {
                throw quxlang::semantic_compilation_error("Mach-O symbol precedes its input section: " + symbol.name);
            }
            return section_base_address(id) + (symbol.value - original_section_address);
        }

        /** Resolves one canonical global symbol to its selected definition or common allocation. */
        auto resolved_symbol_address(std::string const& name) const -> std::uint64_t
        {
            std::map< std::string, macho_resolved_symbol >::const_iterator found = global_symbols.find(name);
            if (found == global_symbols.end())
            {
                throw quxlang::semantic_compilation_error("Undefined Mach-O symbol: " + name);
            }
            macho_resolved_symbol const& symbol = found->second;
            if (symbol.definition.has_value())
            {
                return input_symbol_address(symbol.definition->first, symbol.definition->second);
            }
            if (symbol.common && common_output_section_index.has_value())
            {
                return output_sections.at(*common_output_section_index).virtual_address + symbol.common_offset;
            }
            throw quxlang::semantic_compilation_error("Undefined Mach-O symbol: " + name);
        }

        /** Returns the final executable address of one configured import stub. */
        auto dynamic_import_stub_address(std::string const& relocation_symbol_name) const -> std::uint64_t
        {
            std::map< std::string, macho_dynamic_import_layout >::const_iterator found =
                dynamic_imports.find(relocation_symbol_name);
            if (found == dynamic_imports.end() || !import_stubs_output_section_index.has_value())
            {
                throw quxlang::semantic_compilation_error("Mach-O dynamic import has no allocated stub: " +
                                                          relocation_symbol_name);
            }
            return output_sections.at(*import_stubs_output_section_index).virtual_address +
                   found->second.stub_index * dynamic_import_stub_size();
        }

        /** Resolves one global or object-local relocation symbol identity. */
        auto symbol_reference_address(macho_symbol_reference const& reference) const -> std::uint64_t
        {
            if (!reference.global_name.empty())
            {
                return resolved_symbol_address(reference.global_name);
            }
            return input_symbol_address(reference.object_index, reference.symbol_index);
        }

        /** Returns the final virtual address of a previously allocated GOT slot. */
        auto got_slot_address(macho_symbol_reference const& reference) const -> std::uint64_t
        {
            std::map< macho_symbol_reference, macho_got_slot >::const_iterator found = got_slots.find(reference);
            if (found == got_slots.end() || !got_output_section_index.has_value())
            {
                throw quxlang::semantic_compilation_error("Mach-O relocation refers to an unallocated GOT symbol");
            }
            return output_sections.at(*got_output_section_index).virtual_address + found->second.slot_index * 8;
        }

        /** Verifies that one binary field lies completely within its byte buffer. */
        static void validate_byte_range(std::vector< std::byte > const& bytes, std::uint64_t offset, std::uint64_t size)
        {
            if (offset > bytes.size() || size > bytes.size() - offset)
            {
                throw quxlang::semantic_compilation_error("Mach-O linker binary field is out of range");
            }
        }

        /** Reads one little-endian 32-bit value from a validated byte range. */
        static auto read_u32(std::vector< std::byte > const& bytes, std::uint64_t offset) -> std::uint32_t
        {
            validate_byte_range(bytes, offset, 4);
            return std::to_integer< std::uint32_t >(bytes.at(offset)) |
                   (std::to_integer< std::uint32_t >(bytes.at(offset + 1)) << 8) |
                   (std::to_integer< std::uint32_t >(bytes.at(offset + 2)) << 16) |
                   (std::to_integer< std::uint32_t >(bytes.at(offset + 3)) << 24);
        }

        /** Reads one little-endian 64-bit value from a validated byte range. */
        static auto read_u64(std::vector< std::byte > const& bytes, std::uint64_t offset) -> std::uint64_t
        {
            validate_byte_range(bytes, offset, 8);
            std::uint64_t result = 0;
            for (std::size_t byte_index = 0; byte_index < 8; ++byte_index)
            {
                result |= std::uint64_t(std::to_integer< std::uint8_t >(bytes.at(offset + byte_index)))
                          << (byte_index * 8);
            }
            return result;
        }

        /** Writes one little-endian 32-bit value into a validated byte range. */
        static void write_u32(std::vector< std::byte >& bytes, std::uint64_t offset, std::uint32_t value)
        {
            validate_byte_range(bytes, offset, 4);
            for (std::size_t byte_index = 0; byte_index < 4; ++byte_index)
            {
                bytes.at(offset + byte_index) = static_cast< std::byte >((value >> (byte_index * 8)) & 0xff);
            }
        }

        /** Writes one little-endian 64-bit value into a validated byte range. */
        static void write_u64(std::vector< std::byte >& bytes, std::uint64_t offset, std::uint64_t value)
        {
            validate_byte_range(bytes, offset, 8);
            for (std::size_t byte_index = 0; byte_index < 8; ++byte_index)
            {
                bytes.at(offset + byte_index) = static_cast< std::byte >((value >> (byte_index * 8)) & 0xff);
            }
        }

        /** Reports whether a signed integer can be represented in bit_count bits. */
        static auto signed_value_fits(std::int64_t value, std::size_t bit_count) -> bool
        {
            if (bit_count >= 64)
            {
                return true;
            }
            std::int64_t minimum = -(std::int64_t{1} << (bit_count - 1));
            std::int64_t maximum = (std::int64_t{1} << (bit_count - 1)) - 1;
            return value >= minimum && value <= maximum;
        }

        /** Sign-extends the 24-bit payload encoded by ARM64_RELOC_ADDEND. */
        static auto sign_extend_24(std::uint32_t value) -> std::int64_t
        {
            value &= 0x00ffffff;
            if ((value & 0x00800000) != 0)
            {
                value |= 0xff000000;
            }
            return static_cast< std::int32_t >(value);
        }

        /** Writes resolved pointer values into synthetic GOT slots and records their rebases. */
        void populate_got()
        {
            if (!got_output_section_index.has_value())
            {
                return;
            }
            macho_output_section& got = output_sections.at(*got_output_section_index);
            for (std::pair< macho_symbol_reference const, macho_got_slot > const& entry : got_slots)
            {
                if (!entry.first.global_name.empty() && dynamic_imports.contains(entry.first.global_name))
                {
                    continue;
                }
                write_u64(got.contents, entry.second.slot_index * 8, symbol_reference_address(entry.first));
                rebase_addresses.push_back(got.virtual_address + entry.second.slot_index * 8);
            }
        }

        /** Encodes each import stub to jump through its dyld-bound GOT slot. */
        void populate_dynamic_import_stubs()
        {
            if (!import_stubs_output_section_index.has_value())
            {
                return;
            }
            macho_output_section& stubs = output_sections.at(*import_stubs_output_section_index);
            for (std::pair< std::string const, macho_dynamic_import_layout > const& entry : dynamic_imports)
            {
                macho_symbol_reference reference{.global_name = entry.first};
                std::uint64_t got_address = got_slot_address(reference);
                std::uint64_t stub_address = dynamic_import_stub_address(entry.first);
                std::uint64_t stub_offset = entry.second.stub_index * dynamic_import_stub_size();
                if (machine.cpu_type == quxlang::cpu::arm_64)
                {
                    std::int64_t target_page = static_cast< std::int64_t >(got_address & ~std::uint64_t{0xfff});
                    std::int64_t stub_page = static_cast< std::int64_t >(stub_address & ~std::uint64_t{0xfff});
                    std::int64_t page_delta = target_page - stub_page;
                    if (!signed_value_fits(page_delta, 35))
                    {
                        throw quxlang::semantic_compilation_error("Mach-O ARM64 import stub is out of GOT range");
                    }
                    std::uint64_t page_bits = static_cast< std::uint64_t >(page_delta);
                    std::uint32_t adrp = 0x90000010 | (static_cast< std::uint32_t >((page_bits >> 12) & 0x3) << 29) |
                                         (static_cast< std::uint32_t >((page_bits >> 14) & 0x7ffff) << 5);
                    std::uint64_t page_offset = got_address & 0xfff;
                    if ((page_offset & 7) != 0)
                    {
                        throw quxlang::semantic_compilation_error("Mach-O ARM64 import GOT slot is unaligned");
                    }
                    std::uint32_t load = 0xf9400210 | (static_cast< std::uint32_t >(page_offset >> 3) << 10);
                    write_u32(stubs.contents, stub_offset, adrp);
                    write_u32(stubs.contents, stub_offset + 4, load);
                    write_u32(stubs.contents, stub_offset + 8, 0xd61f0200);
                    continue;
                }

                std::int64_t displacement =
                    static_cast< std::int64_t >(got_address) - static_cast< std::int64_t >(stub_address + 6);
                if (!signed_value_fits(displacement, 32))
                {
                    throw quxlang::semantic_compilation_error("Mach-O x86-64 import stub is out of GOT range");
                }
                validate_byte_range(stubs.contents, stub_offset, 6);
                stubs.contents.at(stub_offset) = static_cast< std::byte >(0xff);
                stubs.contents.at(stub_offset + 1) = static_cast< std::byte >(0x25);
                write_u32(stubs.contents, stub_offset + 2, static_cast< std::uint32_t >(displacement));
            }
        }

        /** Resolves one non-external relocation's 1-based input section ordinal. */
        auto local_relocation_section_id(std::size_t object_index, llvm::MachO::relocation_info relocation) const
            -> macho_input_section_id
        {
            if (relocation.r_extern || relocation.r_symbolnum == 0)
            {
                throw quxlang::semantic_compilation_error("Mach-O local relocation has an invalid section ordinal");
            }
            return macho_input_section_id{
                .object_index = object_index,
                .section_ordinal = relocation.r_symbolnum,
            };
        }

        /** Reads the signed addend embedded at a relocation location. */
        auto embedded_addend(std::vector< std::byte > const& contents, std::uint64_t offset, std::uint8_t length) const
            -> std::int64_t
        {
            if (length == 0)
            {
                validate_byte_range(contents, offset, 1);
                return static_cast< std::int8_t >(std::to_integer< std::uint8_t >(contents.at(offset)));
            }
            if (length == 2)
            {
                return static_cast< std::int32_t >(read_u32(contents, offset));
            }
            if (length == 3)
            {
                return static_cast< std::int64_t >(read_u64(contents, offset));
            }
            throw quxlang::semantic_compilation_error("Mach-O relocation has an unsupported width");
        }

        /** Returns the extra signed-relocation bias encoded by an x86-64 relocation type. */
        static auto x86_pcrel_offset(std::uint8_t relocation_type) -> std::int64_t
        {
            switch (relocation_type)
            {
            case llvm::MachO::X86_64_RELOC_SIGNED_1:
                return 1;
            case llvm::MachO::X86_64_RELOC_SIGNED_2:
                return 2;
            case llvm::MachO::X86_64_RELOC_SIGNED_4:
                return 4;
            default:
                return 0;
            }
        }

        /** Resolves the direct target virtual address selected by one relocation. */
        auto relocation_target_address(std::size_t object_index, llvm::MachO::relocation_info relocation) const
            -> std::uint64_t
        {
            if (relocation.r_extern)
            {
                if (relocation.r_symbolnum >= input_symbols.at(object_index).size())
                {
                    throw quxlang::semantic_compilation_error("Mach-O relocation has an invalid external symbol index");
                }
                macho_input_symbol const& symbol = input_symbols.at(object_index).at(relocation.r_symbolnum);
                if ((symbol.type & llvm::MachO::N_EXT) == 0)
                {
                    return input_symbol_address(object_index, relocation.r_symbolnum);
                }
                std::string name = canonical_symbol_name(symbol.name);
                if (dynamic_imports.contains(name))
                {
                    return dynamic_import_stub_address(name);
                }
                return resolved_symbol_address(name);
            }
            return section_base_address(local_relocation_section_id(object_index, relocation));
        }

        /** Writes a relocation result after validating its encoded width and signedness. */
        void write_relocation_value(std::vector< std::byte >& contents, std::uint64_t offset, std::uint8_t length,
                                    std::uint64_t value, bool require_signed) const
        {
            if (length == 0)
            {
                if ((require_signed && !signed_value_fits(static_cast< std::int64_t >(value), 8)) ||
                    (!require_signed && value > std::numeric_limits< std::uint8_t >::max()))
                {
                    throw quxlang::semantic_compilation_error("Mach-O relocation does not fit in 8 bits");
                }
                validate_byte_range(contents, offset, 1);
                contents.at(offset) = static_cast< std::byte >(value & 0xff);
                return;
            }
            if (length == 2)
            {
                if ((require_signed && !signed_value_fits(static_cast< std::int64_t >(value), 32)) ||
                    (!require_signed && value > std::numeric_limits< std::uint32_t >::max()))
                {
                    throw quxlang::semantic_compilation_error("Mach-O relocation does not fit in 32 bits");
                }
                write_u32(contents, offset, static_cast< std::uint32_t >(value));
                return;
            }
            if (length == 3)
            {
                write_u64(contents, offset, value);
                return;
            }
            throw quxlang::semantic_compilation_error("Mach-O relocation has an unsupported width");
        }

        /** Applies one ARM64 relocation to its merged output section. */
        void apply_arm64_relocation(std::size_t object_index, llvm::MachO::relocation_info relocation,
                                    std::int64_t paired_addend, std::vector< std::byte >& contents,
                                    std::uint64_t location_offset, std::uint64_t location_address)
        {
            std::uint8_t relocation_type = relocation.r_type;
            if (relocation_type == llvm::MachO::ARM64_RELOC_TLVP_LOAD_PAGE21 ||
                relocation_type == llvm::MachO::ARM64_RELOC_TLVP_LOAD_PAGEOFF12)
            {
                throw quxlang::semantic_compilation_error("Mach-O thread-local relocations are not supported yet");
            }

            bool uses_got = relocation_uses_got(machine.cpu_type, relocation_type);
            std::uint64_t value = 0;
            if (uses_got)
            {
                if (!relocation.r_extern)
                {
                    throw quxlang::semantic_compilation_error("Mach-O ARM64 GOT relocation must be external");
                }
                value = got_slot_address(relocation_symbol_reference(object_index, relocation));
            }
            else
            {
                value = relocation_target_address(object_index, relocation);
            }

            if (relocation_type == llvm::MachO::ARM64_RELOC_UNSIGNED)
            {
                std::int64_t addend = embedded_addend(contents, location_offset, relocation.r_length);
                if (!relocation.r_extern)
                {
                    macho_input_section_id target_id = local_relocation_section_id(object_index, relocation);
                    addend -= static_cast< std::int64_t >(input_section_address(target_id));
                }
                value += addend;
                write_relocation_value(contents, location_offset, relocation.r_length, value, false);
                if (relocation.r_length == 3)
                {
                    rebase_addresses.push_back(location_address);
                }
                return;
            }

            value += paired_addend;
            std::uint32_t instruction = read_u32(contents, location_offset);
            if (relocation_type == llvm::MachO::ARM64_RELOC_BRANCH26)
            {
                std::int64_t delta = static_cast< std::int64_t >(value - location_address);
                if ((delta & 3) != 0 || !signed_value_fits(delta, 28))
                {
                    throw quxlang::semantic_compilation_error("Mach-O ARM64 branch target is out of range");
                }
                std::uint32_t encoded = instruction | (static_cast< std::uint32_t >(delta >> 2) & 0x03ffffff);
                write_u32(contents, location_offset, encoded);
                return;
            }
            if (relocation_type == llvm::MachO::ARM64_RELOC_PAGE21 ||
                relocation_type == llvm::MachO::ARM64_RELOC_GOT_LOAD_PAGE21)
            {
                std::int64_t target_page = static_cast< std::int64_t >(value & ~std::uint64_t{0xfff});
                std::int64_t location_page = static_cast< std::int64_t >(location_address & ~std::uint64_t{0xfff});
                std::int64_t delta = target_page - location_page;
                if (!signed_value_fits(delta, 35))
                {
                    throw quxlang::semantic_compilation_error("Mach-O ARM64 page relocation is out of range");
                }
                std::uint64_t bits = static_cast< std::uint64_t >(delta);
                std::uint32_t encoded = instruction | (static_cast< std::uint32_t >((bits >> 12) & 0x3) << 29) |
                                        (static_cast< std::uint32_t >((bits >> 14) & 0x7ffff) << 5);
                write_u32(contents, location_offset, encoded);
                return;
            }
            if (relocation_type == llvm::MachO::ARM64_RELOC_PAGEOFF12 ||
                relocation_type == llvm::MachO::ARM64_RELOC_GOT_LOAD_PAGEOFF12)
            {
                std::size_t scale = 0;
                if ((instruction & 0x3b000000) == 0x39000000)
                {
                    scale = instruction >> 30;
                    if (scale == 0 && (instruction & 0x04800000) == 0x04800000)
                    {
                        scale = 4;
                    }
                }
                std::uint64_t scale_size = std::uint64_t{1} << scale;
                if ((value & (scale_size - 1)) != 0)
                {
                    throw quxlang::semantic_compilation_error("Mach-O ARM64 page-offset relocation is unaligned");
                }
                std::uint32_t immediate = static_cast< std::uint32_t >((value >> scale) & (0xfff >> scale));
                write_u32(contents, location_offset, instruction | (immediate << 10));
                return;
            }
            if (relocation_type == llvm::MachO::ARM64_RELOC_POINTER_TO_GOT)
            {
                std::uint64_t relocated = relocation.r_pcrel ? value - location_address : value;
                write_relocation_value(contents, location_offset, relocation.r_length, relocated, relocation.r_pcrel);
                return;
            }

            throw quxlang::semantic_compilation_error("Unsupported ARM64 Mach-O relocation type " +
                                                      std::to_string(relocation_type));
        }

        /** Applies one x86-64 relocation to its merged output section. */
        void apply_x86_64_relocation(std::size_t object_index, macho_input_section_id source_section_id,
                                     llvm::MachO::relocation_info relocation, std::vector< std::byte >& contents,
                                     std::uint64_t location_offset, std::uint64_t location_address)
        {
            if (relocation.r_type == llvm::MachO::X86_64_RELOC_TLV)
            {
                throw quxlang::semantic_compilation_error("Mach-O thread-local relocations are not supported yet");
            }

            std::int64_t addend =
                embedded_addend(contents, location_offset, relocation.r_length) + x86_pcrel_offset(relocation.r_type);
            std::uint64_t value = 0;
            if (relocation_uses_got(machine.cpu_type, relocation.r_type))
            {
                if (!relocation.r_extern)
                {
                    throw quxlang::semantic_compilation_error("Mach-O x86-64 GOT relocation must be external");
                }
                value = got_slot_address(relocation_symbol_reference(object_index, relocation));
                value += addend;
            }
            else if (relocation.r_extern)
            {
                value = relocation_target_address(object_index, relocation) + addend;
            }
            else
            {
                macho_input_section_id target_section_id = local_relocation_section_id(object_index, relocation);
                if (relocation.r_pcrel)
                {
                    std::uint64_t width = std::uint64_t{1} << relocation.r_length;
                    std::int64_t input_referent_offset =
                        static_cast< std::int64_t >(input_section_address(source_section_id) + relocation.r_address +
                                                    width) +
                        addend - static_cast< std::int64_t >(input_section_address(target_section_id));
                    value = section_base_address(target_section_id) + input_referent_offset;
                }
                else
                {
                    std::int64_t input_referent_offset =
                        addend - static_cast< std::int64_t >(input_section_address(target_section_id));
                    value = section_base_address(target_section_id) + input_referent_offset;
                }
            }

            bool supported = relocation.r_type == llvm::MachO::X86_64_RELOC_UNSIGNED ||
                             relocation.r_type == llvm::MachO::X86_64_RELOC_SIGNED ||
                             relocation.r_type == llvm::MachO::X86_64_RELOC_BRANCH ||
                             relocation.r_type == llvm::MachO::X86_64_RELOC_GOT_LOAD ||
                             relocation.r_type == llvm::MachO::X86_64_RELOC_GOT ||
                             relocation.r_type == llvm::MachO::X86_64_RELOC_SIGNED_1 ||
                             relocation.r_type == llvm::MachO::X86_64_RELOC_SIGNED_2 ||
                             relocation.r_type == llvm::MachO::X86_64_RELOC_SIGNED_4;
            if (!supported)
            {
                throw quxlang::semantic_compilation_error("Unsupported x86-64 Mach-O relocation type " +
                                                          std::to_string(relocation.r_type));
            }

            if (relocation.r_pcrel)
            {
                std::uint64_t width = std::uint64_t{1} << relocation.r_length;
                value -= location_address + width + x86_pcrel_offset(relocation.r_type);
            }
            write_relocation_value(contents, location_offset, relocation.r_length, value,
                                   relocation.r_type != llvm::MachO::X86_64_RELOC_UNSIGNED);
            if (relocation.r_type == llvm::MachO::X86_64_RELOC_UNSIGNED && !relocation.r_pcrel &&
                relocation.r_length == 3)
            {
                rebase_addresses.push_back(location_address);
            }
        }

        /** Applies one paired subtractor relocation expression. */
        void apply_subtractor_relocation(std::size_t object_index, llvm::MachO::relocation_info subtractor,
                                         llvm::MachO::relocation_info minuend, std::vector< std::byte >& contents,
                                         std::uint64_t location_offset)
        {
            if (!subtractor.r_extern || subtractor.r_address != minuend.r_address ||
                subtractor.r_length != minuend.r_length)
            {
                throw quxlang::semantic_compilation_error("Malformed Mach-O subtractor relocation pair");
            }
            std::int64_t addend = embedded_addend(contents, location_offset, subtractor.r_length);
            std::uint64_t subtrahend = relocation_target_address(object_index, subtractor);
            std::uint64_t minuend_address = relocation_target_address(object_index, minuend);
            std::uint64_t value = minuend_address + addend - subtrahend;
            write_relocation_value(contents, location_offset, subtractor.r_length, value, false);
        }

        /** Applies every retained input relocation after final addresses are known. */
        void apply_relocations()
        {
            for (std::size_t object_index = 0; object_index < input_objects.size(); ++object_index)
            {
                llvm::object::MachOObjectFile const& macho = object(object_index);
                for (llvm::object::SectionRef section : macho.sections())
                {
                    std::uint32_t ordinal = macho.getSectionID(section) + 1;
                    macho_input_section_id source_id{.object_index = object_index, .section_ordinal = ordinal};
                    std::map< macho_input_section_id, macho_section_placement >::const_iterator placement =
                        section_placements.find(source_id);
                    if (placement == section_placements.end())
                    {
                        continue;
                    }
                    macho_output_section& output = output_sections.at(placement->second.output_section_index);
                    if (output.zerofill)
                    {
                        if (section.relocation_begin() != section.relocation_end())
                        {
                            throw quxlang::semantic_compilation_error("Mach-O zero-fill section contains relocations");
                        }
                        continue;
                    }

                    std::vector< llvm::MachO::relocation_info > relocations;
                    for (llvm::object::RelocationRef relocation : section.relocations())
                    {
                        relocations.push_back(plain_relocation(macho, relocation));
                    }
                    for (std::size_t relocation_index = 0; relocation_index < relocations.size(); ++relocation_index)
                    {
                        llvm::MachO::relocation_info relocation = relocations.at(relocation_index);
                        if (relocation.r_address < 0)
                        {
                            throw quxlang::semantic_compilation_error("Scattered Mach-O relocations are not supported");
                        }
                        std::int64_t paired_addend = 0;
                        if (machine.cpu_type == quxlang::cpu::arm_64 &&
                            relocation.r_type == llvm::MachO::ARM64_RELOC_ADDEND)
                        {
                            paired_addend = sign_extend_24(relocation.r_symbolnum);
                            if (++relocation_index >= relocations.size())
                            {
                                throw quxlang::semantic_compilation_error(
                                    "Mach-O ARM64 addend relocation has no paired relocation");
                            }
                            relocation = relocations.at(relocation_index);
                        }

                        std::uint8_t subtractor_type = machine.cpu_type == quxlang::cpu::arm_64
                                                           ? llvm::MachO::ARM64_RELOC_SUBTRACTOR
                                                           : llvm::MachO::X86_64_RELOC_SUBTRACTOR;
                        std::uint8_t unsigned_type = machine.cpu_type == quxlang::cpu::arm_64
                                                         ? llvm::MachO::ARM64_RELOC_UNSIGNED
                                                         : llvm::MachO::X86_64_RELOC_UNSIGNED;
                        std::uint64_t location_offset =
                            placement->second.output_offset + static_cast< std::uint32_t >(relocation.r_address);
                        std::uint64_t location_address = output.virtual_address + location_offset;
                        if (relocation.r_type == subtractor_type)
                        {
                            if (++relocation_index >= relocations.size() ||
                                relocations.at(relocation_index).r_type != unsigned_type)
                            {
                                throw quxlang::semantic_compilation_error(
                                    "Mach-O subtractor relocation has no unsigned pair");
                            }
                            apply_subtractor_relocation(object_index, relocation, relocations.at(relocation_index),
                                                        output.contents, location_offset);
                            continue;
                        }

                        if (machine.cpu_type == quxlang::cpu::arm_64)
                        {
                            apply_arm64_relocation(object_index, relocation, paired_addend, output.contents,
                                                   location_offset, location_address);
                        }
                        else
                        {
                            apply_x86_64_relocation(object_index, source_id, relocation, output.contents,
                                                    location_offset, location_address);
                        }
                    }
                }
            }
        }

        /** Appends one unsigned LEB128 value to a dyld opcode stream. */
        static void append_uleb128(std::vector< std::byte >& bytes, std::uint64_t value)
        {
            do
            {
                std::uint8_t byte = static_cast< std::uint8_t >(value & 0x7f);
                value >>= 7;
                if (value != 0)
                {
                    byte |= 0x80;
                }
                bytes.push_back(static_cast< std::byte >(byte));
            } while (value != 0);
        }

        /** Encodes a Mach-O dyld opcode and its four-bit immediate operand. */
        static auto encode_dyld_opcode(std::uint8_t opcode, std::uint8_t immediate) -> std::byte
        {
            constexpr std::uint8_t immediate_mask = 0x0f;
            if ((opcode & immediate_mask) != 0 || immediate > immediate_mask)
            {
                throw quxlang::compiler_bug("Mach-O dyld opcode encoding is invalid");
            }
            return static_cast< std::byte >(opcode | immediate);
        }

        /** Appends dyld opcodes for one contiguous range of pointer rebases. */
        void append_rebase_range(std::uint64_t address, std::size_t count)
        {
            if (count == 0 || count > (std::numeric_limits< std::uint64_t >::max() - address) / 8)
            {
                throw quxlang::semantic_compilation_error("Mach-O pointer rebase range is invalid");
            }
            std::uint64_t range_end = address + std::uint64_t(count) * 8;
            std::uint8_t segment_index = 0;
            std::uint64_t segment_offset = 0;
            if (address >= text_segment.virtual_address &&
                range_end <= text_segment.virtual_address + text_segment.file_size)
            {
                segment_index = 1;
                segment_offset = address - text_segment.virtual_address;
            }
            else if (address >= data_segment.virtual_address &&
                     range_end <= data_segment.virtual_address + data_segment.file_size)
            {
                segment_index = 2;
                segment_offset = address - data_segment.virtual_address;
            }
            else
            {
                throw quxlang::semantic_compilation_error("Mach-O rebase location is outside a file-backed segment");
            }
            if ((address & 7) != 0)
            {
                throw quxlang::semantic_compilation_error("Mach-O pointer rebase location is not naturally aligned");
            }

            rebase_data.push_back(encode_dyld_opcode(
                static_cast< std::uint8_t >(llvm::MachO::REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB), segment_index));
            append_uleb128(rebase_data, segment_offset);
            if (count <= llvm::MachO::REBASE_IMMEDIATE_MASK)
            {
                rebase_data.push_back(encode_dyld_opcode(
                    static_cast< std::uint8_t >(llvm::MachO::REBASE_OPCODE_DO_REBASE_IMM_TIMES),
                    static_cast< std::uint8_t >(count)));
            }
            else
            {
                rebase_data.push_back(static_cast< std::byte >(llvm::MachO::REBASE_OPCODE_DO_REBASE_ULEB_TIMES));
                append_uleb128(rebase_data, count);
            }
        }

        /** Encodes dyld bindings from libSystem exports into import GOT slots. */
        void encode_bind_data()
        {
            if (dynamic_imports.empty())
            {
                return;
            }
            bind_data.push_back(encode_dyld_opcode(
                static_cast< std::uint8_t >(llvm::MachO::BIND_OPCODE_SET_DYLIB_ORDINAL_IMM), 1));
            bind_data.push_back(encode_dyld_opcode(
                static_cast< std::uint8_t >(llvm::MachO::BIND_OPCODE_SET_TYPE_IMM),
                static_cast< std::uint8_t >(llvm::MachO::BIND_TYPE_POINTER)));
            for (std::pair< std::string const, macho_dynamic_import_layout > const& entry : dynamic_imports)
            {
                std::uint8_t flags = entry.second.import.optional
                                         ? static_cast< std::uint8_t >(llvm::MachO::BIND_SYMBOL_FLAGS_WEAK_IMPORT)
                                         : 0;
                bind_data.push_back(encode_dyld_opcode(
                    static_cast< std::uint8_t >(llvm::MachO::BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM), flags));
                std::string dyld_symbol_name = "_" + entry.second.import.symbol_name;
                for (char character : dyld_symbol_name)
                {
                    bind_data.push_back(static_cast< std::byte >(character));
                }
                bind_data.push_back(std::byte{});

                macho_symbol_reference reference{.global_name = entry.first};
                std::uint64_t address = got_slot_address(reference);
                if (address < data_segment.virtual_address ||
                    address + 8 > data_segment.virtual_address + data_segment.file_size)
                {
                    throw quxlang::semantic_compilation_error(
                        "Mach-O dynamic import GOT slot is outside the file-backed data segment");
                }
                bind_data.push_back(encode_dyld_opcode(
                    static_cast< std::uint8_t >(llvm::MachO::BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB), 2));
                append_uleb128(bind_data, address - data_segment.virtual_address);
                bind_data.push_back(static_cast< std::byte >(llvm::MachO::BIND_OPCODE_DO_BIND));
            }
            bind_data.push_back(static_cast< std::byte >(llvm::MachO::BIND_OPCODE_DONE));
            bind_data.resize(static_cast< std::size_t >(align_up(bind_data.size(), 8)), std::byte{});
        }

        /** Encodes dyld metadata and places the ad-hoc signature within __LINKEDIT. */
        void finalize_linkedit_layout()
        {
            std::sort(rebase_addresses.begin(), rebase_addresses.end());
            rebase_addresses.erase(std::unique(rebase_addresses.begin(), rebase_addresses.end()),
                                   rebase_addresses.end());

            rebase_data.push_back(encode_dyld_opcode(
                static_cast< std::uint8_t >(llvm::MachO::REBASE_OPCODE_SET_TYPE_IMM),
                static_cast< std::uint8_t >(llvm::MachO::REBASE_TYPE_POINTER)));
            for (std::size_t range_begin = 0; range_begin < rebase_addresses.size();)
            {
                std::size_t range_end = range_begin + 1;
                while (range_end < rebase_addresses.size() &&
                       rebase_addresses.at(range_end) == rebase_addresses.at(range_end - 1) + 8)
                {
                    ++range_end;
                }
                append_rebase_range(rebase_addresses.at(range_begin), range_end - range_begin);
                range_begin = range_end;
            }
            rebase_data.push_back(static_cast< std::byte >(llvm::MachO::REBASE_OPCODE_DONE));
            rebase_data.resize(static_cast< std::size_t >(align_up(rebase_data.size(), 8)), std::byte{});

            encode_bind_data();
            std::uint64_t linkedit_cursor = rebase_data_offset + rebase_data.size();
            if (!bind_data.empty())
            {
                bind_data_offset = align_up(linkedit_cursor, 8);
                linkedit_cursor = bind_data_offset + bind_data.size();
            }

            code_signature_offset = align_up(linkedit_cursor, 16);
            std::uint64_t block_count = (code_signature_offset + 0xfff) / 0x1000;
            std::uint64_t blob_headers_size =
                align_up(sizeof(llvm::MachO::CS_SuperBlob) + sizeof(llvm::MachO::CS_BlobIndex), 8);
            std::uint64_t fixed_headers_size = blob_headers_size + sizeof(llvm::MachO::CS_CodeDirectory);
            std::uint64_t all_headers_size = align_up(fixed_headers_size + options.signature_identifier.size() + 1, 16);
            code_signature_size = all_headers_size + block_count * 32;
            std::uint64_t linkedit_file_size = code_signature_offset + code_signature_size - rebase_data_offset;
            linkedit_segment = macho_segment_layout{
                .file_offset = rebase_data_offset,
                .file_size = linkedit_file_size,
                .virtual_address = text_segment.virtual_address + rebase_data_offset,
                .memory_size = align_up(linkedit_file_size, page_size()),
            };
        }

        /** Writes one big-endian 32-bit code-signature field. */
        static void write_be32(std::vector< std::byte >& bytes, std::uint64_t offset, std::uint32_t value)
        {
            validate_byte_range(bytes, offset, 4);
            for (std::size_t byte_index = 0; byte_index < 4; ++byte_index)
            {
                bytes.at(offset + byte_index) = static_cast< std::byte >((value >> ((3 - byte_index) * 8)) & 0xff);
            }
        }

        /** Writes one big-endian 64-bit code-signature field. */
        static void write_be64(std::vector< std::byte >& bytes, std::uint64_t offset, std::uint64_t value)
        {
            validate_byte_range(bytes, offset, 8);
            for (std::size_t byte_index = 0; byte_index < 8; ++byte_index)
            {
                bytes.at(offset + byte_index) = static_cast< std::byte >((value >> ((7 - byte_index) * 8)) & 0xff);
            }
        }

        /** Writes one null-padded 16-byte Mach-O section or segment name. */
        static void write_fixed_name(std::vector< std::byte >& bytes, std::uint64_t offset, std::string const& name)
        {
            if (name.size() > 16)
            {
                throw quxlang::semantic_compilation_error("Mach-O fixed name exceeds 16 bytes: " + name);
            }
            validate_byte_range(bytes, offset, 16);
            for (std::size_t character_index = 0; character_index < name.size(); ++character_index)
            {
                bytes.at(offset + character_index) = static_cast< std::byte >(name.at(character_index));
            }
        }

        /** Converts a validated power-of-two alignment into a Mach-O exponent. */
        static auto alignment_power(std::uint64_t alignment) -> std::uint32_t
        {
            std::uint32_t result = 0;
            while ((std::uint64_t{1} << result) < alignment)
            {
                ++result;
            }
            return result;
        }

        /** Writes one LC_SEGMENT_64 command and all of its section records. */
        auto write_segment_command(std::vector< std::byte >& result, std::uint64_t command_offset,
                                   std::string const& name, macho_segment_layout const& segment,
                                   std::uint32_t maximum_protection, std::uint32_t initial_protection,
                                   std::vector< std::size_t > const& section_indices) const -> std::uint64_t
        {
            std::uint64_t command_size =
                sizeof(llvm::MachO::segment_command_64) + section_indices.size() * sizeof(llvm::MachO::section_64);
            write_u32(result, command_offset, llvm::MachO::LC_SEGMENT_64);
            write_u32(result, command_offset + 4, static_cast< std::uint32_t >(command_size));
            write_fixed_name(result, command_offset + 8, name);
            write_u64(result, command_offset + 24, segment.virtual_address);
            write_u64(result, command_offset + 32, segment.memory_size);
            write_u64(result, command_offset + 40, segment.file_offset);
            write_u64(result, command_offset + 48, segment.file_size);
            write_u32(result, command_offset + 56, maximum_protection);
            write_u32(result, command_offset + 60, initial_protection);
            write_u32(result, command_offset + 64, static_cast< std::uint32_t >(section_indices.size()));
            write_u32(result, command_offset + 68, 0);

            std::uint64_t section_header_offset = command_offset + sizeof(llvm::MachO::segment_command_64);
            for (std::size_t output_index : section_indices)
            {
                macho_output_section const& section = output_sections.at(output_index);
                write_fixed_name(result, section_header_offset, section.section_name);
                write_fixed_name(result, section_header_offset + 16, section.segment_name);
                write_u64(result, section_header_offset + 32, section.virtual_address);
                write_u64(result, section_header_offset + 40, section.memory_size);
                write_u32(result, section_header_offset + 48, static_cast< std::uint32_t >(section.file_offset));
                write_u32(result, section_header_offset + 52, alignment_power(section.alignment));
                write_u32(result, section_header_offset + 56, 0);
                write_u32(result, section_header_offset + 60, 0);
                write_u32(result, section_header_offset + 64, section.flags);
                write_u32(result, section_header_offset + 68, 0);
                write_u32(result, section_header_offset + 72, 0);
                write_u32(result, section_header_offset + 76, 0);
                section_header_offset += sizeof(llvm::MachO::section_64);
            }
            return command_offset + command_size;
        }

        /** Writes the embedded ad-hoc SHA-256 code signature over the completed image prefix. */
        void write_code_signature(std::vector< std::byte >& result) const
        {
            std::uint64_t blob_headers_size =
                align_up(sizeof(llvm::MachO::CS_SuperBlob) + sizeof(llvm::MachO::CS_BlobIndex), 8);
            std::uint64_t fixed_headers_size = blob_headers_size + sizeof(llvm::MachO::CS_CodeDirectory);
            std::uint64_t all_headers_size = align_up(fixed_headers_size + options.signature_identifier.size() + 1, 16);
            std::uint32_t code_slot_count = static_cast< std::uint32_t >((code_signature_offset + 0xfff) / 0x1000);
            std::uint64_t signature = code_signature_offset;
            std::uint64_t directory = signature + blob_headers_size;

            write_be32(result, signature, llvm::MachO::CSMAGIC_EMBEDDED_SIGNATURE);
            write_be32(result, signature + 4, static_cast< std::uint32_t >(code_signature_size));
            write_be32(result, signature + 8, 1);
            write_be32(result, signature + 12, llvm::MachO::CSSLOT_CODEDIRECTORY);
            write_be32(result, signature + 16, static_cast< std::uint32_t >(blob_headers_size));

            write_be32(result, directory, llvm::MachO::CSMAGIC_CODEDIRECTORY);
            write_be32(result, directory + 4, static_cast< std::uint32_t >(code_signature_size - blob_headers_size));
            write_be32(result, directory + 8, llvm::MachO::CS_SUPPORTSEXECSEG);
            write_be32(result, directory + 12, llvm::MachO::CS_ADHOC | llvm::MachO::CS_LINKER_SIGNED);
            write_be32(result, directory + 16, static_cast< std::uint32_t >(all_headers_size - blob_headers_size));
            write_be32(result, directory + 20, sizeof(llvm::MachO::CS_CodeDirectory));
            write_be32(result, directory + 24, 0);
            write_be32(result, directory + 28, code_slot_count);
            write_be32(result, directory + 32, static_cast< std::uint32_t >(code_signature_offset));
            validate_byte_range(result, directory + 36, 4);
            result.at(directory + 36) = static_cast< std::byte >(32);
            result.at(directory + 37) = static_cast< std::byte >(llvm::MachO::kSecCodeSignatureHashSHA256);
            result.at(directory + 38) = std::byte{};
            result.at(directory + 39) = static_cast< std::byte >(12);
            write_be32(result, directory + 40, 0);
            write_be32(result, directory + 44, 0);
            write_be32(result, directory + 48, 0);
            write_be32(result, directory + 52, 0);
            write_be64(result, directory + 56, 0);
            write_be64(result, directory + 64, text_segment.file_offset);
            write_be64(result, directory + 72, text_segment.file_size);
            write_be64(result, directory + 80, llvm::MachO::CS_EXECSEG_MAIN_BINARY);

            std::uint64_t identifier_offset = directory + sizeof(llvm::MachO::CS_CodeDirectory);
            validate_byte_range(result, identifier_offset, options.signature_identifier.size() + 1);
            for (std::size_t character_index = 0; character_index < options.signature_identifier.size();
                 ++character_index)
            {
                result.at(identifier_offset + character_index) =
                    static_cast< std::byte >(options.signature_identifier.at(character_index));
            }

            std::uint64_t hash_offset = signature + all_headers_size;
            for (std::uint32_t slot = 0; slot < code_slot_count; ++slot)
            {
                std::uint64_t page_offset = std::uint64_t(slot) * 0x1000;
                std::uint64_t page_byte_count = std::min< std::uint64_t >(0x1000, code_signature_offset - page_offset);
                llvm::ArrayRef< std::uint8_t > page(
                    reinterpret_cast< std::uint8_t const* >(result.data() + page_offset),
                    static_cast< std::size_t >(page_byte_count));
                std::array< std::uint8_t, 32 > hash = llvm::SHA256::hash(page);
                validate_byte_range(result, hash_offset + std::uint64_t(slot) * hash.size(), hash.size());
                for (std::size_t byte_index = 0; byte_index < hash.size(); ++byte_index)
                {
                    result.at(hash_offset + std::uint64_t(slot) * hash.size() + byte_index) =
                        static_cast< std::byte >(hash.at(byte_index));
                }
            }
        }

        /** Serializes final headers, sections, dyld metadata, and code signature. */
        auto build_executable_image() const -> std::vector< std::byte >
        {
            if (rebase_data_offset > std::numeric_limits< std::uint32_t >::max() ||
                rebase_data.size() > std::numeric_limits< std::uint32_t >::max() ||
                bind_data_offset > std::numeric_limits< std::uint32_t >::max() ||
                bind_data.size() > std::numeric_limits< std::uint32_t >::max() ||
                code_signature_offset > std::numeric_limits< std::uint32_t >::max() ||
                code_signature_size > std::numeric_limits< std::uint32_t >::max())
            {
                throw quxlang::semantic_compilation_error("Mach-O executable exceeds 32-bit load-command offsets");
            }
            std::vector< std::byte > result(static_cast< std::size_t >(code_signature_offset + code_signature_size),
                                            std::byte{});
            std::uint32_t command_count = 10;
            write_u32(result, 0, llvm::MachO::MH_MAGIC_64);
            write_u32(result, 4, expected_cpu_type());
            write_u32(result, 8, expected_cpu_subtype());
            write_u32(result, 12, llvm::MachO::MH_EXECUTE);
            write_u32(result, 16, command_count);
            write_u32(result, 20, static_cast< std::uint32_t >(load_commands_size()));
            write_u32(result, 24,
                      llvm::MachO::MH_NOUNDEFS | llvm::MachO::MH_DYLDLINK | llvm::MachO::MH_TWOLEVEL |
                          llvm::MachO::MH_PIE);
            write_u32(result, 28, 0);

            std::vector< std::size_t > text_sections;
            std::vector< std::size_t > data_sections;
            for (std::size_t output_index = 0; output_index < output_sections.size(); ++output_index)
            {
                if (output_sections.at(output_index).segment_name == "__TEXT")
                {
                    text_sections.push_back(output_index);
                }
                else
                {
                    data_sections.push_back(output_index);
                }
            }

            std::uint64_t command_offset = sizeof(llvm::MachO::mach_header_64);
            macho_segment_layout pagezero{
                .file_offset = 0,
                .file_size = 0,
                .virtual_address = 0,
                .memory_size = text_segment.virtual_address,
            };
            command_offset = write_segment_command(result, command_offset, "__PAGEZERO", pagezero, 0, 0, {});
            command_offset = write_segment_command(result, command_offset, "__TEXT", text_segment, 5, 5, text_sections);
            command_offset = write_segment_command(result, command_offset, "__DATA", data_segment, 3, 3, data_sections);
            command_offset = write_segment_command(result, command_offset, "__LINKEDIT", linkedit_segment, 1, 1, {});

            write_u32(result, command_offset, llvm::MachO::LC_DYLD_INFO_ONLY);
            write_u32(result, command_offset + 4, sizeof(llvm::MachO::dyld_info_command));
            write_u32(result, command_offset + 8, static_cast< std::uint32_t >(rebase_data_offset));
            write_u32(result, command_offset + 12, static_cast< std::uint32_t >(rebase_data.size()));
            write_u32(result, command_offset + 16, static_cast< std::uint32_t >(bind_data_offset));
            write_u32(result, command_offset + 20, static_cast< std::uint32_t >(bind_data.size()));
            write_u32(result, command_offset + 24, 0);
            write_u32(result, command_offset + 28, 0);
            write_u32(result, command_offset + 32, 0);
            write_u32(result, command_offset + 36, 0);
            write_u32(result, command_offset + 40, 0);
            write_u32(result, command_offset + 44, 0);
            command_offset += sizeof(llvm::MachO::dyld_info_command);

            write_u32(result, command_offset, llvm::MachO::LC_LOAD_DYLINKER);
            write_u32(result, command_offset + 4, 32);
            write_u32(result, command_offset + 8, 12);
            std::string dylinker = "/usr/lib/dyld";
            validate_byte_range(result, command_offset + 12, dylinker.size() + 1);
            for (std::size_t character_index = 0; character_index < dylinker.size(); ++character_index)
            {
                result.at(command_offset + 12 + character_index) =
                    static_cast< std::byte >(dylinker.at(character_index));
            }
            command_offset += 32;

            write_u32(result, command_offset, llvm::MachO::LC_BUILD_VERSION);
            write_u32(result, command_offset + 4, sizeof(llvm::MachO::build_version_command));
            write_u32(result, command_offset + 8, llvm::MachO::PLATFORM_MACOS);
            write_u32(result, command_offset + 12, 0x000b0000);
            write_u32(result, command_offset + 16, 0x000b0000);
            write_u32(result, command_offset + 20, 0);
            command_offset += sizeof(llvm::MachO::build_version_command);

            write_u32(result, command_offset, llvm::MachO::LC_MAIN);
            write_u32(result, command_offset + 4, sizeof(llvm::MachO::entry_point_command));
            write_u64(result, command_offset + 8, entry_address - text_segment.virtual_address);
            write_u64(result, command_offset + 16, 0);
            command_offset += sizeof(llvm::MachO::entry_point_command);

            write_u32(result, command_offset, llvm::MachO::LC_LOAD_DYLIB);
            write_u32(result, command_offset + 4, 56);
            write_u32(result, command_offset + 8, 24);
            write_u32(result, command_offset + 12, 2);
            write_u32(result, command_offset + 16, 0);
            write_u32(result, command_offset + 20, 0x00010000);
            std::string libsystem = "/usr/lib/libSystem.B.dylib";
            validate_byte_range(result, command_offset + 24, libsystem.size() + 1);
            for (std::size_t character_index = 0; character_index < libsystem.size(); ++character_index)
            {
                result.at(command_offset + 24 + character_index) =
                    static_cast< std::byte >(libsystem.at(character_index));
            }
            command_offset += 56;

            write_u32(result, command_offset, llvm::MachO::LC_CODE_SIGNATURE);
            write_u32(result, command_offset + 4, sizeof(llvm::MachO::linkedit_data_command));
            write_u32(result, command_offset + 8, static_cast< std::uint32_t >(code_signature_offset));
            write_u32(result, command_offset + 12, static_cast< std::uint32_t >(code_signature_size));
            command_offset += sizeof(llvm::MachO::linkedit_data_command);
            if (command_offset != sizeof(llvm::MachO::mach_header_64) + load_commands_size())
            {
                throw quxlang::compiler_bug("Mach-O load-command layout size mismatch");
            }

            for (macho_output_section const& section : output_sections)
            {
                if (section.zerofill)
                {
                    continue;
                }
                validate_byte_range(result, section.file_offset, section.contents.size());
                std::copy(section.contents.begin(), section.contents.end(), result.begin() + section.file_offset);
            }
            validate_byte_range(result, rebase_data_offset, rebase_data.size());
            std::copy(rebase_data.begin(), rebase_data.end(), result.begin() + rebase_data_offset);
            if (!bind_data.empty())
            {
                validate_byte_range(result, bind_data_offset, bind_data.size());
                std::copy(bind_data.begin(), bind_data.end(), result.begin() + bind_data_offset);
            }
            write_code_signature(result);
            return result;
        }
    };
} // namespace quxlang::detail

auto quxlang::macho_linker::link_macos_executable(machine_target_info const& machine,
                                                  std::vector< std::vector< std::byte > > const& object_files,
                                                  std::string const& entry_symbol,
                                                  macho_link_options const& options) const -> std::vector< std::byte >
{
    detail::macho_link_session session(machine, object_files, entry_symbol, options);
    return session.link();
}
