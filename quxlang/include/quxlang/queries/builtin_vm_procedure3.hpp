// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_BUILTIN_VM_PROCEDURE3_HEADER_GUARD
#define QUXLANG_QUERIES_BUILTIN_VM_PROCEDURE3_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/vmir2/vmir2.hpp>


namespace quxlang
{
    /// Generates a built-in routine
    /// @pre The input references a routine which is already determined to be builtin and require generation.
    /// @param input The instanciation_reference to the built-in routine
    /// @retval output The resulting generated routine.
    struct builtin_vm_procedure3_query
    {
        static constexpr auto query_id = "builtin_vm_procedure3";
        using input_type = instanciation_reference;
        using output_type = vmir2::functanoid_routine3;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_BUILTIN_VM_PROCEDURE3_HEADER_GUARD
