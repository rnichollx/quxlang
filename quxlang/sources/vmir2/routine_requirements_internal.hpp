// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_ROUTINE_REQUIREMENTS_INTERNAL_HEADER_GUARD
#define QUXLANG_VMIR2_ROUTINE_REQUIREMENTS_INTERNAL_HEADER_GUARD

#include <quxlang/vmir2/routine_requirements.hpp>

namespace quxlang::vmir2::detail
{
    /** Returns the concrete functanoid represented by a symbol, if any. */
    auto concrete_functanoid_from_symbol(type_symbol const& symbol) -> std::optional< type_symbol >;
    /** Adds a required concrete functanoid or reports an invalid VMIR reference. */
    void add_functanoid(std::set< type_symbol >& result, type_symbol const& symbol);
    /** Adds a symbol when it represents a concrete functanoid. */
    void add_optional_functanoid(std::set< type_symbol >& result, type_symbol const& symbol);
    /** Returns whether a symbol names function-local static data. */
    auto symbol_is_localdata_root(type_symbol const& symbol) -> bool;
    /** Returns whether a symbol is a functanoid reference. */
    auto symbol_is_functanoid_reference(type_symbol const& symbol) -> bool;
    /** Adds a non-local, non-functanoid antestatal global. */
    void add_antestatal_global(std::set< type_symbol >& result, type_symbol const& symbol);
    /** Adds a type and recursively contained type components. */
    void add_type_and_components(std::set< type_symbol >& result, type_symbol type);
    /** Returns whether a type can require a separately queried layout. */
    auto type_might_have_layout(type_symbol const& type) -> bool;
    /** Adds types exposed by a routine's reachable surface. */
    void add_routine_surface_types(std::set< type_symbol >& result, functanoid_routine3 const& routine,
                                   std::set< local_index > const& locals, std::set< static_snapshot_ref > const& snapshots);
    /** Removes slot wrappers from a supplied antestatal type. */
    auto normalized_antestatal_type(std::optional< type_symbol > type) -> std::optional< type_symbol >;
    /** Adds functanoids reachable through an antestatal access path. */
    void add_functanoids_from_antestatal_access(std::set< type_symbol >& result, antestatal_access const& access);
    /** Adds functanoids represented by an antestatal value. */
    void add_functanoids_from_antestatal_value(std::set< type_symbol >& result, antestatal_value const& value,
                                               std::optional< type_symbol > type);
    /** Adds globals reachable through an antestatal access path. */
    void add_antestatal_globals_from_antestatal_access(std::set< type_symbol >& result, antestatal_access const& access);
    /** Adds globals represented by an antestatal value. */
    void add_antestatal_globals_from_antestatal_value(std::set< type_symbol >& result, antestatal_value const& value,
                                                      std::optional< type_symbol > type);
    /** Adds local snapshots reachable through an antestatal access path. */
    void add_static_snapshots_from_antestatal_access(std::set< static_snapshot_ref >& result, antestatal_access const& access);
    /** Adds local snapshots represented by an antestatal value. */
    void add_static_snapshots_from_antestatal_value(std::set< static_snapshot_ref >& result, antestatal_value const& value);
} // namespace quxlang::vmir2::detail

#endif // QUXLANG_VMIR2_ROUTINE_REQUIREMENTS_INTERNAL_HEADER_GUARD
