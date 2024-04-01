//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef RPNX_QUXLANG_VARIANTS_HEADER
#define RPNX_QUXLANG_VARIANTS_HEADER

#include <rpnx/variant.hpp>

namespace quxlang
{
    struct ast2_named_global;
    struct ast2_named_member;
    struct ast2_include_if;

    using ast2_named_declaration = rpnx::variant< ast2_named_global, ast2_named_member >;

    using ast2_top_declaration = rpnx::variant< ast2_named_global, ast2_named_member, ast2_include_if >;

} // namespace quxlang

#endif // RPNX_QUXLANG_VARIANTS_HEADER
