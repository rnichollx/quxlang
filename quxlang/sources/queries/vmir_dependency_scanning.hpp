// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_QUERIES_VMIR_DEPENDENCY_SCANNING_HEADER_GUARD
#define QUXLANG_SOURCES_QUERIES_VMIR_DEPENDENCY_SCANNING_HEADER_GUARD

#include <quxlang/data/dependencies.hpp>
#include <quxlang/data/functanoid_requirements.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang::detail
{
    /** Scans one generated VMIR routine for a query-owned cached dependency result. */
    auto scan_routine_dependencies(vmir2::functanoid_routine3 const& routine, dependency_set set) -> dependencies;

    /** Scans one constexpr function-local static value for a query-owned cached dependency result. */
    auto scan_constexpr_static_dependencies(antestatal_value const& value, type_symbol const& type) -> dependencies;
} // namespace quxlang::detail

#endif // QUXLANG_SOURCES_QUERIES_VMIR_DEPENDENCY_SCANNING_HEADER_GUARD
