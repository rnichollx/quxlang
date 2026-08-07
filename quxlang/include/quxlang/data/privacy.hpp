// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_PRIVACY_HEADER_GUARD
#define QUXLANG_DATA_PRIVACY_HEADER_GUARD

#include <quxlang/ast2/source_location.hpp>
#include <quxlang/data/basic_types.hpp>

#include <cstdint>
#include <optional>
#include <set>

/** Selects whether access applies only to a declaration or to its qualified lookup path. */
RPNX_ENUM(quxlang, declaration_access_kind, std::uint8_t, selected_declaration, lookup_path);

namespace quxlang
{
    /** Canonical contexts granted access by a declaration's PRIVATE annotation. */
    struct resolved_privacy_scope
    {
        /// Canonical contexts that independently grant access.
        std::set< type_symbol > contexts;
        /// Location used when reporting access diagnostics.
        std::optional< source_location > declaration_location;

        RPNX_MEMBER_METADATA(resolved_privacy_scope, contexts, declaration_location);
    };

    /** Identifies an accessing context and the declaration it selected. */
    struct declaration_access_request
    {
        /// Canonical context performing the access.
        type_symbol accessor_context;
        /// Canonical declaration or selected overload being accessed.
        type_symbol selected_declaration;
        /// Determines whether canonical lexical parents named by lookup must also be accessible.
        declaration_access_kind kind = declaration_access_kind::selected_declaration;

        RPNX_MEMBER_METADATA(declaration_access_request, accessor_context, selected_declaration, kind);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_PRIVACY_HEADER_GUARD
