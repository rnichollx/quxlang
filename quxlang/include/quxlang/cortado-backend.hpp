// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CORTADO_BACKEND_HEADER_GUARD
#define QUXLANG_CORTADO_BACKEND_HEADER_GUARD

#include <quxlang/cortado-backend-types.hpp>

#include <cstddef>
#include <vector>

namespace quxlang::cortado_backend
{
    /** Generates and validates a deterministic Java 17 JAR for one Cortado compilation packet. */
    auto emit_jar(cortado_compilable_unit const& input) -> std::vector< std::byte >;
} // namespace quxlang::cortado_backend

#endif // QUXLANG_CORTADO_BACKEND_HEADER_GUARD
