//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef AST2_ENTITY_HEADER_GUARD
#define AST2_ENTITY_HEADER_GUARD

#include "ast2_named_declaration.hpp"
#include "quxlang/asm/asm.hpp"
#include "quxlang/data/variants.hpp"
#include <boost/variant.hpp>
#include <cinttypes>
#include <quxlang/ast2/ast2_function_arg.hpp>
#include <quxlang/ast2/ast2_function_delegate.hpp>
#include <rpnx/resolver_utilities.hpp>

namespace quxlang
{
    struct ast2_namespace_declaration;
    struct ast2_variable_declaration;
    struct ast2_file_declaration;
    struct ast2_class_declaration;
    struct ast2_function_declaration;
    struct ast2_template_declaration;
    struct ast2_function_template_declaration;
    struct ast2_module_declaration;
    struct ast2_extern;
    struct ast2_asm_procedure_declaration;

    using ast2_declarable = rpnx::variant< std::monostate, ast2_namespace_declaration, ast2_variable_declaration, ast2_template_declaration, ast2_class_declaration, ast2_function_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    struct ast2_functum;
    struct ast2_templex;

    using ast2_map_entity = rpnx::variant< std::monostate, ast2_functum, ast2_class_declaration, ast2_variable_declaration, ast2_templex, ast2_module_declaration, ast2_namespace_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    using ast2_node = rpnx::variant< std::monostate, ast2_functum, ast2_class_declaration, ast2_variable_declaration, ast2_templex, ast2_module_declaration, ast2_namespace_declaration, ast2_function_declaration, ast2_template_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    using ast2_temploid = rpnx::variant< std::monostate, ast2_class_declaration, ast2_function_declaration >;

    using ast2_templexoid = rpnx::variant< std::monostate, ast2_functum, ast2_templex >;

    struct ast2_extern
    {
        std::string lang;
        std::string symbol;
        std::vector< ast2_function_arg > args;

        auto operator<=>(const ast2_extern& other) const
        {
            return rpnx::compare(lang, other.lang, symbol, other.symbol, args, other.args);
        }
    };

    struct ast2_procedure_ref
    {
        std::string cc;
        type_symbol functanoid;

        auto operator<=>(const ast2_procedure_ref& other) const
        {
            return rpnx::compare(cc, other.cc, functanoid, other.functanoid);
        }
    };

    struct ast2_argument_interface
    {
        std::string register_name;
        type_symbol type;

        auto operator<=>(const ast2_argument_interface& other) const
        {
            return rpnx::compare(register_name, other.register_name, type, other.type);
        }
    };

    using ast2_asm_operand_component = rpnx::variant< std::string, ast2_extern, ast2_procedure_ref >;

    struct ast2_asm_operand
    {
        std::vector< ast2_asm_operand_component > components;

        auto operator<=>(const ast2_asm_operand& other) const
        {
            return rpnx::compare(components, other.components);
        }
    };

    struct ast2_asm_instruction
    {
        std::string opcode_mnemonic;
        std::vector< ast2_asm_operand > operands;

        auto operator<=>(const ast2_asm_instruction& other) const
        {
            return rpnx::compare(opcode_mnemonic, other.opcode_mnemonic, operands, other.operands);
        }
    };

    struct ast2_asm_procedure
    {
        std::string name;
        std::vector< asm_instruction > instructions;
    };

    // ast2_asm_callable defines the interface by which an asm_procedure can be called
    // if it can be called from within the language
    struct ast2_asm_callable
    {
        std::string calling_conv;
        std::vector< ast2_argument_interface > args;

        std::set< std::string > clobber;
        std::optional< type_symbol > return_type;

        std::strong_ordering operator<=>(const ast2_asm_callable& other) const
        {
            return rpnx::compare(calling_conv, other.calling_conv, args, other.args, clobber, other.clobber, return_type, other.return_type);
        }
    };

    struct ast2_asm_procedure_declaration
    {
        std::vector< ast2_asm_instruction > instructions;
        std::optional< std::string > linkname;
        std::vector< ast2_asm_callable > callable_interfaces;
        std::vector< type_symbol > imports;

        std::strong_ordering operator<=>(const ast2_asm_procedure_declaration& other) const
        {
            return rpnx::compare(instructions, other.instructions, linkname, other.linkname, callable_interfaces, other.callable_interfaces);
        }
    };

    struct ast2_functum
    {
        std::vector< ast2_function_declaration > functions;

        std::strong_ordering operator<=>(const ast2_functum& other) const
        {
            return rpnx::compare(functions, other.functions);
        }
    };

    struct ast2_namespace_declaration
    {
        std::vector< ast2_top_declaration > declarations;

        RPNX_MEMBER_METADATA(ast2_namespace_declaration, declarations);
    };

    struct ast2_variable_declaration
    {
        type_symbol type;
        std::optional< std::size_t > offset;
        std::optional< expression > include_if;

        RPNX_MEMBER_METADATA(ast2_variable_declaration, type, offset, include_if);
    };

    struct ast2_class_declaration
    {
        std::vector< ast2_top_declaration > declarations;
        std::set< std::string > class_keywords;

        RPNX_MEMBER_METADATA(ast2_class_declaration, declarations, class_keywords);
    };

    struct ast2_template_declaration
    {
        std::vector< type_symbol > m_template_args;
        ast2_class_declaration m_class;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(ast2_template_declaration, m_template_args, m_class, priority);
    };

    struct ast2_templex
    {
        std::vector< ast2_template_declaration > templates;

        RPNX_MEMBER_METADATA(ast2_templex, templates);
    };

    struct ast2_function_declaration
    {
        std::vector< ast2_function_arg > args;
        std::optional< type_symbol > return_type;
        std::optional< type_symbol > this_type;
        std::vector< ast2_function_delegate > delegates;
        std::optional< std::int64_t > priority;
        function_block body;

        RPNX_MEMBER_METADATA(ast2_function_declaration, args, return_type, this_type, delegates, priority, body);
    };

    struct ast2_named_global
    {
        std::string name;
        ast2_declarable declaration;

        RPNX_MEMBER_METADATA(ast2_named_global, name, declaration);
    };

    struct ast2_named_member
    {
        std::string name;
        ast2_declarable declaration;

        RPNX_MEMBER_METADATA(ast2_named_member, name, declaration);
    };

    struct ast2_include_if;

    struct ast2_include_if
    {
        expression condition;
        ast2_named_declaration declaration;

        RPNX_MEMBER_METADATA(ast2_include_if, condition, declaration);
    };

    struct ast2_file_declaration
    {
        std::string filename;
        std::string module_name;
        std::map< std::string, std::string > imports;
        // std::vector< std::pair< std::string, ast2_declarable > > globals;
        std::vector< ast2_top_declaration > declarations;

        RPNX_MEMBER_METADATA(ast2_file_declaration, filename, module_name, imports, declarations);
    };

    struct ast2_module_declaration
    {
        std::string module_name;
        std::map< std::string, std::string > imports;
        std::vector< ast2_top_declaration > declarations;

        RPNX_MEMBER_METADATA(ast2_module_declaration, module_name, imports, declarations);
    };

    struct ast2_declarations
    {
        std::vector< std::pair< std::string, ast2_declarable > > globals;
        std::vector< std::pair< std::string, ast2_declarable > > members;

        RPNX_MEMBER_METADATA(ast2_declarations, globals, members);
    };

    std::string to_string(ast2_function_declaration const& ref);
    std::string to_string(ast2_declarable const& ref);

} // namespace quxlang

#endif // AST2_ENTITY_HEADER_GUARD