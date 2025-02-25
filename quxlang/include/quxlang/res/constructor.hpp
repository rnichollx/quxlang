//
// Created by Ryan Nicholl on 2024-12-21.
//

#ifndef RPNX_QUXLANG_CONSTRUCTOR_HEADER
#define RPNX_QUXLANG_CONSTRUCTOR_HEADER


#include "quxlang/data/expression.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    // Determines if a type requires compiler-generated destructor.
    // Returns true if:
    // - The type has no user-defined destructor AND
    // - The type is not trivially destructible
    QUX_CO_RESOLVER(requires_gen_default_dtor, type_symbol, bool);

    // Determines if a type requires compiler-generated constructor.
    // Returns true if:
    // - The type has no user-defined default constructor AND
    // - The type is not trivially constructible
    QUX_CO_RESOLVER(requires_gen_default_ctor, type_symbol, bool);

    // Looks up any user-defined destructor for the type.
    // Returns a reference to the destructor if found, or nullopt if none exists.
    QUX_CO_RESOLVER(user_default_dtor, type_symbol, std::optional<instanciation_reference>);

    // Looks up any user-defined default constructor for the type.
    // Returns a reference to the constructor if found, or nullopt if none exists.
    QUX_CO_RESOLVER(user_default_ctor, type_symbol, std::optional<instanciation_reference>);

    // Determines if a type is trivially destructible.
    // Returns true if destroying an instance requires no code generation
    // (i.e., no cleanup needed for the type and all its members)
    QUX_CO_RESOLVER(trivially_destructible, type_symbol, bool);

    // Determines if a type is trivially constructible.
    // Returns true if constructing an instance requires no code generation
    // (i.e., no initialization needed for the type and all its members)
    QUX_CO_RESOLVER(trivially_constructible, type_symbol, bool);

    // Gets the effective destructor for a type, whether user-defined or compiler-generated.
    // Returns a reference to the destructor implementation that should be used.
    QUX_CO_RESOLVER(default_dtor, type_symbol, std::optional<instanciation_reference>);

    // Gets the effective default constructor for a type, whether user-defined or compiler-generated.
    // Returns a reference to the constructor implementation that should be used.
    QUX_CO_RESOLVER(default_ctor, type_symbol, std::optional<instanciation_reference>);
}

#endif // RPNX_QUXLANG_CONSTRUCTOR_HEADER
