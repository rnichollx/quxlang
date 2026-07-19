// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_LINKER_ELF_LINKER_INTERNAL_HEADER_GUARD
#define QUXLANG_SOURCES_LINKER_ELF_LINKER_INTERNAL_HEADER_GUARD

namespace quxlang::detail
{
    /** Stores one allocated input section after linker layout. */
    struct linked_section;
    /** Stores one output load segment. */
    struct load_segment;
    /** Stores the output TLS image bounds. */
    struct tls_segment;
    /** Stores one global-offset-table allocation. */
    struct got_slot;
    /** Stores one symbol emitted into the linked image. */
    struct output_symbol;
    /** Stores one common-symbol allocation. */
    struct common_symbol_allocation;
    /** Stores runtime import indices assigned during layout. */
    struct dynamic_import_layout;
    /** Owns one in-memory ELF link operation. */
    class elf_link_session;
} // namespace quxlang::detail

#endif // QUXLANG_SOURCES_LINKER_ELF_LINKER_INTERNAL_HEADER_GUARD
