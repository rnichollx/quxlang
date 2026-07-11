// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_ROUTINE_REQUIREMENTS_HEADER_GUARD
#define QUXLANG_VMIR2_ROUTINE_REQUIREMENTS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/vmir2/vmir2.hpp>

#include <optional>
#include <set>

namespace quxlang::vmir2
{
    /**
     * Returns concrete functanoids directly referenced by one VMIR2 routine.
     */
    auto directly_instantiated_functanoids(functanoid_routine3 const& routine) -> std::set< type_symbol >;

    /**
     * Returns concrete functanoids directly referenced by one antestatal value.
     */
    auto directly_instantiated_functanoids(antestatal_value const& value, std::optional< type_symbol > type = std::nullopt) -> std::set< type_symbol >;

    /**
     * Returns antestatal global data roots directly referenced by one VMIR2 routine.
     */
    auto directly_referenced_antestatal_globals(functanoid_routine3 const& routine) -> std::set< type_symbol >;

    /**
     * Returns mutable global roots directly referenced by one VMIR2 routine.
     */
    auto directly_referenced_global_roots(functanoid_routine3 const& routine) -> std::set< type_symbol >;

    /**
     * Returns antestatal global data roots directly referenced by one antestatal value.
     */
    auto directly_referenced_antestatal_globals(antestatal_value const& value, std::optional< type_symbol > type = std::nullopt) -> std::set< type_symbol >;

    /**
     * Returns type-placement inputs directly required by one VMIR2 routine.
     */
    auto directly_required_type_placements(functanoid_routine3 const& routine) -> std::set< type_symbol >;

    /**
     * Returns class-layout inputs directly required by one VMIR2 routine.
     */
    auto directly_required_struct_layouts(functanoid_routine3 const& routine) -> std::set< type_symbol >;
} // namespace quxlang::vmir2

#endif // QUXLANG_VMIR2_ROUTINE_REQUIREMENTS_HEADER_GUARD
