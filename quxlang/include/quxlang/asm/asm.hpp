// Copyright 2024, 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_ASM_ASM_HEADER_GUARD
#define QUXLANG_ASM_ASM_HEADER_GUARD

#include <quxlang/data/fwd.hpp>

#include <optional>
#include <set>
#include <string>
#include <vector>

#include <rpnx/compare.hpp>
#include <rpnx/variant.hpp>
#include <rpnx/macros.hpp>

namespace quxlang
{

    struct asm_instruction
    {
        std::string opcode_mnemonic;
        std::vector< std::string > operands;

        RPNX_MEMBER_METADATA(asm_instruction, opcode_mnemonic, operands);
    };

    struct asm_label
    {
        std::string name;

        RPNX_MEMBER_METADATA(asm_label, name);
    };

    using asm_statement = rpnx::variant< asm_instruction, asm_label >;

    struct asm_argument_binding
    {
        /// Target register name for this callable argument.
        std::optional< std::string > register_name;
        /// Source-level parameter type corresponding to the bound register.
        type_symbol type;

        RPNX_MEMBER_METADATA(asm_argument_binding, register_name, type);
    };

    struct asm_callable
    {
        /// Calling convention tag declared on the CALLABLE surface.
        std::string calling_conv;
        /// Ordered argument-to-register bindings.
        std::vector< asm_argument_binding > args;
        /// Explicit clobber registers declared by the asm callable surface.
        std::set< std::string > clobber;
        /// Optional return register.
        std::optional< std::string > return_register_name;
        /// Optional return type.
        std::optional< type_symbol > return_type;

        RPNX_MEMBER_METADATA(asm_callable, calling_conv, args, clobber, return_register_name, return_type);
    };

    struct asm_procedure
    {
        /// Target architecture tag as written in the source declaration, e.g. X64 or ARM.
        std::string architecture;
        /// Link-visible symbol name emitted for this assembly routine.
        std::string name;
        /// Selected callable surface for this instantiated asm routine, if it is callable from Qux.
        std::optional< asm_callable > callable_interface;
        /// Ordered statements that form the body of this assembly routine.
        std::vector< asm_statement > instructions;

        RPNX_MEMBER_METADATA(asm_procedure, architecture, name, callable_interface, instructions);
    };


} // namespace quxlang

#endif // RPNX_QUXLANG_ASM_HEADER
