//
// Created by Ryan Nicholl on 2024-12-21.
//

#ifndef RPNX_QUXLANG_CONSTRUCTOR_HEADER
#define RPNX_QUXLANG_CONSTRUCTOR_HEADER

#include "quxlang/data/expression.hpp"
#include <quxlang/res/resolver.hpp>

#include "quxlang/data/builtin_functions.hpp"

namespace quxlang
{
    // Determines if a type requires compiler-generated destructor.
    // Returns true if:
    // - The type has no user-defined destructor AND
    // - The type is not trivially destructible
    QUX_CO_RESOLVER(class_requires_gen_default_dtor, type_symbol, bool);

    // Determines if a type requires compiler-generated constructor.
    // Returns true if:
    // - The type has no user-defined default constructor AND
    // - The type is not trivially constructible
    // - NO_DEFAULT_CONSTRUCTOR is not set, and
    // - NO_IMPLICIT_CONSTRUCTORS is not set
    QUX_CO_RESOLVER(class_requires_gen_default_ctor, type_symbol, bool);

    // Determines if a type requires compiler-generated copy constructor.
    // Returns true if:
    // - The type has no user-defined copy constructor AND
    // - The type is not trivially copyable
    QUX_CO_RESOLVER(class_requires_gen_copy_ctor, type_symbol, bool);

    QUX_CO_RESOLVER(class_requires_gen_move_ctor, type_symbol, bool);

    QUX_CO_RESOLVER(class_requires_gen_swap, type_symbol, bool);

    // Looks up any user-defined destructor for the type.
    // Returns a reference to the destructor if found, or nullopt if none exists.
    QUX_CO_RESOLVER(user_default_dtor_exists, type_symbol, bool);

    // Looks up any user-defined default constructor for the type.
    // Returns a reference to the constructor if found, or nullopt if none exists.
    QUX_CO_RESOLVER(user_default_ctor_exists, type_symbol, bool);

    QUX_CO_RESOLVER(user_copy_ctor_exists, type_symbol, bool);

    QUX_CO_RESOLVER(user_move_ctor_exists, type_symbol, bool);

    QUX_CO_RESOLVER(user_swap_exists, type_symbol, bool);

    // Determines if a type is trivially destructible.
    // Returns true if destroying an instance requires no code generation
    // (i.e., no cleanup needed for the type and all its members)
    QUX_CO_RESOLVER(class_trivially_destructible, type_symbol, bool);

    // Determines if a type is trivially constructible.
    // Returns true if constructing an instance requires no code generation
    // (i.e., no initialization needed for the type and all its members)
    QUX_CO_RESOLVER(class_trivially_constructible, type_symbol, bool);

    // Gets the effective destructor for a type, whether user-defined or compiler-generated.
    // Returns a reference to the destructor implementation that should be used.
    QUX_CO_RESOLVER(class_default_dtor, type_symbol, std::optional< instanciation_reference >);

    // Gets the effective default constructor for a type, whether user-defined or compiler-generated.
    // Returns a reference to the constructor implementation that should be used.
    QUX_CO_RESOLVER(class_default_ctor, type_symbol, std::optional< instanciation_reference >);

    QUX_CO_RESOLVER(have_nontrivial_member_dtor, type_symbol, bool);
    QUX_CO_RESOLVER(have_nontrivial_member_ctor, type_symbol, bool);

    QUX_CO_RESOLVER(list_primitive_constructors, type_symbol, std::set< builtin_function_info >);
    QUX_CO_RESOLVER(list_primitive_destructors, type_symbol, std::set< builtin_function_info >);

} // namespace quxlang

#endif // RPNX_QUXLANG_CONSTRUCTOR_HEADER
