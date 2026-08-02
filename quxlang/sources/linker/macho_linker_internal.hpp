// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_LINKER_MACHO_LINKER_INTERNAL_HEADER_GUARD
#define QUXLANG_SOURCES_LINKER_MACHO_LINKER_INTERNAL_HEADER_GUARD

namespace quxlang::detail
{
    /** Identifies one section in one Mach-O input object. */
    struct macho_input_section_id;
    /** Retains one parsed Mach-O input object. */
    struct macho_input_object;
    /** Stores one parsed Mach-O symbol table record. */
    struct macho_input_symbol;
    /** Stores one selected Mach-O symbol definition. */
    struct macho_resolved_symbol;
    /** Locates one input contribution within an output section. */
    struct macho_section_placement;
    /** Stores one allocated input section contribution. */
    struct macho_input_section;
    /** Stores one final Mach-O section. */
    struct macho_output_section;
    /** Identifies one synthetic global-offset-table slot. */
    struct macho_got_slot;
    /** Stores the final stub and GOT allocation for one dynamic import. */
    struct macho_dynamic_import_layout;
    /** Identifies one global or object-local symbol referenced by a relocation. */
    struct macho_symbol_reference;
    /** Stores final segment bounds used by Mach-O load commands. */
    struct macho_segment_layout;
    /** Owns one in-memory Mach-O link operation. */
    class macho_link_session;
} // namespace quxlang::detail

#endif // QUXLANG_SOURCES_LINKER_MACHO_LINKER_INTERNAL_HEADER_GUARD
