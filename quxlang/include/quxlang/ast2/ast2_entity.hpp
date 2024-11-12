// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_AST2_AST2_ENTITY_HEADER_GUARD
#define QUXLANG_AST2_AST2_ENTITY_HEADER_GUARD

#include "ast2_named_declaration.hpp"
#include "quxlang/asm/asm.hpp"
#include "quxlang/data/function_header.hpp"
#include "quxlang/data/variants.hpp"
#include <boost/variant.hpp>
#include <cinttypes>
#include <quxlang/ast2/ast2_function_arg.hpp>
#include <quxlang/ast2/ast2_function_delegate.hpp>
#include <quxlang/macros.hpp>
#include <rpnx/resolver_utilities.hpp>

namespace quxlang
{
    struct ast2_source_location
    {
        std::size_t file_id = {};
        std::size_t begin_index = {};
        std::size_t end_index = {};

        template < typename It >
        void set(It begin, It end)
        {
            // TODO: Do something here later maybe
        }

        RPNX_MEMBER_METADATA(ast2_source_location, file_id, begin_index, end_index);
    };
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
    struct functum;
    struct member_subdeclaroid;
    struct global_subdeclaroid;
    struct ast2_templex;
    struct function;
    struct templex;

    using ast2_declarable [[deprecated("replace with declaroid/symboid etc")]] = rpnx::variant< std::monostate, ast2_namespace_declaration, ast2_variable_declaration, ast2_template_declaration, ast2_class_declaration, ast2_function_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    using declaroid = rpnx::variant< std::monostate, ast2_namespace_declaration, ast2_variable_declaration, ast2_template_declaration, ast2_class_declaration, ast2_function_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    using subdeclaroid = rpnx::variant< member_subdeclaroid, global_subdeclaroid >;

    using ast2_map_entity [[deprecated("replace with declaroid/symboid etc")]] = rpnx::variant< std::monostate, functum, ast2_class_declaration, ast2_variable_declaration, ast2_templex, ast2_module_declaration, ast2_namespace_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    using ast2_node [[deprecated("replace with declaroid/symboid etc")]] = rpnx::variant< std::monostate, functum, ast2_class_declaration, ast2_variable_declaration, ast2_templex, ast2_module_declaration, ast2_namespace_declaration, ast2_function_declaration, ast2_template_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    using ast2_symboid = rpnx::variant< std::monostate, functum, ast2_class_declaration, ast2_variable_declaration, ast2_templex, ast2_module_declaration, ast2_namespace_declaration, ast2_function_declaration, ast2_template_declaration, ast2_extern, ast2_asm_procedure_declaration >;

    using ast2_temploid = rpnx::variant< std::monostate, ast2_class_declaration, ast2_function_declaration >;

    using ast2_templexoid = rpnx::variant< std::monostate, functum, ast2_templex >;

    struct member_subdeclaroid
    {
        declaroid decl;
        std::string name;
        std::optional<expression> include_if;

        RPNX_MEMBER_METADATA(member_subdeclaroid, name, decl, include_if);
    };

    struct global_subdeclaroid
    {

        declaroid decl;
        std::string name;
        std::optional<expression> include_if;

        RPNX_MEMBER_METADATA(global_subdeclaroid, name, decl, include_if);
    };


    struct ast2_procedure_ref
    {
        std::string cc;
        type_symbol functanoid;

        RPNX_MEMBER_METADATA(ast2_procedure_ref, cc, functanoid);
    };

    struct ast2_argument_interface
    {
        std::string register_name;
        type_symbol type;

        RPNX_MEMBER_METADATA(ast2_argument_interface, register_name, type);
    };

    using ast2_asm_operand_component = rpnx::variant< std::string, ast2_extern, ast2_procedure_ref >;

    struct ast2_asm_operand
    {
        std::vector< ast2_asm_operand_component > components;

        RPNX_MEMBER_METADATA(ast2_asm_operand, components);
    };

    struct ast2_asm_instruction
    {
        std::string opcode_mnemonic;
        std::vector< ast2_asm_operand > operands;

        RPNX_MEMBER_METADATA(ast2_asm_instruction, opcode_mnemonic, operands);
    };

    struct ast2_asm_procedure
    {
        std::string name;
        std::vector< asm_instruction > instructions;

        RPNX_MEMBER_METADATA(ast2_asm_procedure, name, instructions);
    };

    // ast2_asm_callable defines the interface by which an asm_procedure can be called
    // if it can be called from within the language
    struct ast2_asm_callable
    {
        std::string calling_conv;
        std::vector< ast2_argument_interface > args;

        std::set< std::string > clobber;
        std::optional< type_symbol > return_type;

        RPNX_MEMBER_METADATA(ast2_asm_callable, calling_conv, args, clobber, return_type);
    };

    struct ast2_asm_procedure_declaration
    {
        std::vector< ast2_asm_instruction > instructions;
        std::optional< std::string > linkname;
        std::vector< ast2_asm_callable > callable_interfaces;
        std::vector< type_symbol > imports;

        RPNX_MEMBER_METADATA(ast2_asm_procedure_declaration, instructions, linkname, callable_interfaces, imports);
    };

    struct ast2_namespace_declaration
    {
        std::vector< subdeclaroid > declarations;

        RPNX_MEMBER_METADATA(ast2_namespace_declaration, declarations);
    };

    struct ast2_variable_declaration
    {
        type_symbol type;
        std::optional< std::size_t > offset;

        RPNX_MEMBER_METADATA(ast2_variable_declaration, type, offset);
    };

    struct ast2_class_declaration
    {
        std::vector< subdeclaroid > declarations;
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

    struct ast2_function_parameter
    {
        ast2_source_location location;
        std::string name;
        std::optional< std::string > api_name;
        type_symbol type;
        std::optional< expression > default_expr;

        RPNX_MEMBER_METADATA(ast2_function_parameter, location, name, api_name, type, default_expr)
    };

    struct ast2_function_header
    {
        ast2_source_location location;
        std::vector< ast2_function_parameter > call_parameters;
        std::optional< std::int64_t > priority;


        RPNX_MEMBER_METADATA(ast2_function_header, location, call_parameters, priority);
    };

    struct parameters
    {
        std::vector<type_symbol> positional;
        std::map<std::string, type_symbol> named;

        RPNX_MEMBER_METADATA(parameters, positional, named);
    };

    struct ast2_function_definition
    {
        ast2_source_location location;
        std::optional< type_symbol > return_type;
        std::vector< ast2_function_delegate > delegates;
        function_block body;

        RPNX_MEMBER_METADATA(ast2_function_definition, location, return_type, delegates, body);
    };

    struct ast2_functum
    {
        std::vector< ast2_function_definition > functions;
        RPNX_MEMBER_METADATA(ast2_functum, functions);
    };

    struct ast2_function_declaration
    {
        ast2_source_location location;
        ast2_function_header header;
        ast2_function_definition definition;

        RPNX_MEMBER_METADATA(ast2_function_declaration, header, definition);
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

    struct [[deprecated("use include_if")]] ast2_include_if
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
        std::vector< subdeclaroid > declarations;

        RPNX_MEMBER_METADATA(ast2_file_declaration, filename, module_name, imports, declarations);
    };

    struct ast2_module_declaration
    {
        std::string module_name;
        std::map< std::string, std::string > imports;
        std::vector< subdeclaroid > declarations;

        RPNX_MEMBER_METADATA(ast2_module_declaration, module_name, imports, declarations);
    };

    struct ast2_declarations
    {
        std::vector< std::pair< std::string, ast2_declarable > > globals;
        std::vector< std::pair< std::string, ast2_declarable > > members;

        RPNX_MEMBER_METADATA(ast2_declarations, globals, members);
    };

    struct function
    {
    };

    struct functum
    {
        std::vector< ast2_function_declaration > functions;

        RPNX_MEMBER_METADATA(functum, functions);
    };

    struct ast2_extern
    {
        std::string lang;
        std::string symbol;
        std::vector< ast2_function_parameter > args;

        QUX_AST_METADATA_NOCONV(extern, lang, symbol, args);
    };

    std::string to_string(ast2_function_declaration const& ref);
    std::string to_string(ast2_declarable const& ref);
    std::string to_string(expression const& ref);

} // namespace quxlang

#endif // AST2_ENTITY_HEADER_GUARD