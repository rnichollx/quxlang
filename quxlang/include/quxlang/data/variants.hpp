// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VARIANTS_HEADER_GUARD
#define QUXLANG_DATA_VARIANTS_HEADER_GUARD

#include <rpnx/variant.hpp>

namespace quxlang
{
    struct ast2_named_global;
    struct ast2_named_member;
    struct ast2_include_if;

    using ast2_named_declaration = rpnx::variant< ast2_named_global, ast2_named_member >;

    using ast2_top_declaration = rpnx::variant< ast2_named_declaration, ast2_include_if >;

} // namespace quxlang

#endif // RPNX_QUXLANG_VARIANTS_HEADER
