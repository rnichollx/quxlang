// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD
#define QUXLANG_RES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    // functum_select_function does overload resolution and chooses a function from a functum based on the given parameters.
    QUX_CO_RESOLVER(functum_select_function, initialization_reference, std::optional< temploid_reference >);

    // functum_exists_and_is_callable_with returns true if a functum both exists and is callable with the given parameters.
    QUX_CO_RESOLVER(functum_exists_and_is_callable_with, initialization_reference, bool);

    // functum_initialize will do overload resolution and create the instanciation_reference for the function.
    // It do the complete overload resolution for selecting the function and also instanciate the
    // functanoid parameters.
    QUX_CO_RESOLVER(functum_initialize, initialization_reference, std::optional< instanciation_reference >);

    // functum_primitive_overloads lists the set of "primitive" overloads of a given functum,
    // primitive functions are a subset of the built-in functions.
    // A primitive function is one that maps directly to a QXVMIR2 instruction.
    // For example, I32::.OPERATOR+(@OTHER I32) is a primitive function, because it
    // maps directly to the IADD instruction.
    // On the other hand, built-in functions like struct default constructors are not usually primitive
    // because they involve a fully generated IR routine instead of a single instruction.
    // Note that this only refers to the QXVMIR2 instruction set, it's possible that some operations
    // which are single instructions in QXVMIR2 (like atomic add) might be multiple instructions in the
    // machine language or even a procedure invoke.
    QUX_CO_RESOLVER(functum_primitive_overloads, type_symbol, std::set< builtin_function_info >);

    // functum_builtin_overloads lists the set of built-in overloads of a given functum.
    // This basically means all functions that are not typed out in the source code,
    // such as default constructors, as well as the set of primitive operations.
    QUX_CO_RESOLVER(functum_builtin_overloads, type_symbol, std::set< temploid_ensig >);

    // functum_overloads lists the set of all overloads of a given functum.
    // This includes both builtin and user-defined overloads.
    QUX_CO_RESOLVER(functum_overloads, type_symbol, std::set< temploid_ensig >);

    // functum_user_overloads lists the set of all user overloads of a given functum.
    // A user overload is a function that is defined in the source code.
    // TODO: This should not exist, since it should return a set, not a list.
    QUX_CO_RESOLVER(functum_user_overloads, type_symbol, std::vector< temploid_ensig >);

    // functum_list_user_ensig_declarations lists all *declared* ensigs for a given functum.
    // A declared ensig is not yet formally typed, for example, a declared ensig might be
    // `#[I32, mytype]` whereas the formal ensig could be `#[I32, MODULE(foo)::mynamespace::mytype]`.
    // Since the compiler tends to pass around formal types rather than context-dependent types,
    // this is only really useful for functum_map_user_formal_ensigs.
    QUX_CO_RESOLVER(functum_list_user_ensig_declarations, type_symbol, std::vector< temploid_ensig >);

    // functum_map_user_formal_ensigs returns a std::map where the key is the formal ensig of a user-defined functum,
    // and the value is the index of the ensig in the list of user-defined ensigs as well as the index
    // of the function declaration in the list of user-defined function declarations.
    using functum_map_formal_ensigs_output_type = std::map< temploid_ensig, std::size_t >;
    QUX_CO_RESOLVER(functum_map_user_formal_ensigs, type_symbol, functum_map_formal_ensigs_output_type);

    // function_declaration returns the AST declaration of a function from its *formal* temploid reference
    QUX_CO_RESOLVER(function_declaration, temploid_reference, std::optional< ast2_function_declaration >);

    // functum_list_user_overload_declarations lists all user-defined function declaration ASTs for the given functum.
    QUX_CO_RESOLVER(functum_list_user_overload_declarations, type_symbol, std::vector< ast2_function_declaration >);

    // list_user_functum_formal_paratypes lists all formal paratypes of a functum
    // TODO: Delete this and make it a call to function_formal_paratype instead?
    QUX_CO_RESOLVER(list_user_functum_formal_paratypes, type_symbol, std::vector< paratype >);



} // namespace quxlang

#endif // RPNX_QUXLANG_FUNCTUM_SELECT_FUNCTION_HEADER
