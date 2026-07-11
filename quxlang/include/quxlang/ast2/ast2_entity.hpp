// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_AST2_AST2_ENTITY_HEADER_GUARD
#define QUXLANG_AST2_AST2_ENTITY_HEADER_GUARD

#include "ast2_named_declaration.hpp"
#include "quxlang/asm/asm.hpp"
#include "quxlang/data/function_header.hpp"
#include "quxlang/data/variants.hpp"

#include <cinttypes>
#include <quxlang/ast2/ast2_function_arg.hpp>
#include <quxlang/ast2/ast2_function_delegate.hpp>
#include "quxlang/ast2/source_location.hpp"

RPNX_ENUM(quxlang, option_kind, std::uint16_t, number, string, boolean);
RPNX_ENUM(quxlang, ast2_test_mode, std::uint16_t, static_only, unit_only, dual);
RPNX_ENUM(quxlang, static_test_expected_mode, std::uint16_t, normal, expect_fail, expect_compilation_failure);
RPNX_ENUM(quxlang, ast2_asm_declaration_kind, std::uint16_t, procedure, inline_function);

namespace quxlang
{
    struct ast2_namespace_declaration;
    struct ast2_variable_declaration;
    struct ast2_file_declaration;
    struct ast2_struct_declaration;
    struct ast2_interface_declaration;
    struct ast2_implementation_declaration;
    struct ast2_enum_declaration;
    struct ast2_flagset_declaration;
    struct ast2_interface_function_declaration;
    struct ast2_function_declaration;
    struct ast2_template_declaration;
    struct ast2_function_template_declaration;
    struct ast2_module_declaration;
    struct ast2_test;
    struct ast2_extern;
    struct ast2_extern_procedure;
    struct ast2_object_ref;
    struct ast2_asm_procedure_declaration;
    struct functum;
    struct member_subdeclaroid;
    struct global_subdeclaroid;
    struct ast2_templex;
    struct function;
    struct templex;
    struct ast2_option;

    using declaroid = rpnx::variant< std::monostate, ast2_namespace_declaration, ast2_variable_declaration, ast2_template_declaration, ast2_struct_declaration, ast2_interface_declaration, ast2_implementation_declaration, ast2_enum_declaration, ast2_flagset_declaration, ast2_function_declaration, ast2_extern, ast2_extern_procedure, ast2_asm_procedure_declaration, ast2_test, ast2_option >;

    using subdeclaroid = rpnx::variant< member_subdeclaroid, global_subdeclaroid >;

    using ast2_symboid = rpnx::variant< std::monostate, functum, ast2_struct_declaration, ast2_interface_declaration, ast2_implementation_declaration, ast2_enum_declaration, ast2_flagset_declaration, ast2_variable_declaration, ast2_templex, ast2_module_declaration, ast2_namespace_declaration, ast2_function_declaration, ast2_template_declaration, ast2_extern, ast2_extern_procedure, ast2_asm_procedure_declaration, ast2_test, ast2_option >;

    using temploid = rpnx::variant< std::monostate, ast2_struct_declaration, ast2_interface_declaration, ast2_implementation_declaration, ast2_enum_declaration, ast2_flagset_declaration, ast2_function_declaration, ast2_variable_declaration >;

    struct member_subdeclaroid
    {
        declaroid decl;
        std::string name;
        std::optional<expression> include_if;
        std::optional< std::string > doc;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(member_subdeclaroid, name, decl, include_if, doc);
    };

    struct global_subdeclaroid
    {
        declaroid decl;
        std::string name;
        std::optional<expression> include_if;
        std::optional< std::string > doc;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(global_subdeclaroid, name, decl, include_if, doc);
    };


    struct ast2_procedure_ref
    {
        std::string cc;
        type_symbol functanoid;

        RPNX_MEMBER_METADATA(ast2_procedure_ref, cc, functanoid);
    };

    struct ast2_object_ref
    {
        type_symbol object;

        RPNX_MEMBER_METADATA(ast2_object_ref, object);
    };

    struct ast2_argument_interface
    {
        /// Optional external call-site name for this asm callable argument.
        std::optional< std::string > api_name;
        /// Optional target register binding for inline asm callable arguments.
        std::optional< std::string > register_name;
        /// Declared source-level parameter type.
        type_symbol type;

        RPNX_MEMBER_METADATA(ast2_argument_interface, api_name, register_name, type);
    };

    using ast2_asm_operand_component = rpnx::variant< std::string, ast2_extern, ast2_procedure_ref, ast2_object_ref >;

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
        std::optional< std::string > return_register_name;
        std::optional< type_symbol > return_type;

        RPNX_MEMBER_METADATA(ast2_asm_callable, calling_conv, args, clobber, return_register_name, return_type);
    };

    struct ast2_asm_procedure_declaration
    {
        ast2_asm_declaration_kind kind = ast2_asm_declaration_kind::procedure;
        std::string architecture;
        std::vector< ast2_asm_instruction > instructions;
        std::vector< ast2_asm_callable > callable_interfaces;
        std::vector< type_symbol > imports;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_asm_procedure_declaration, kind, architecture, instructions, callable_interfaces, imports);
    };

    struct ast2_namespace_declaration
    {
        std::vector< subdeclaroid > declarations;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_namespace_declaration, declarations);
    };

    struct ast2_variable_declaration
    {
        type_symbol type;
        std::set< std::string > keyword_tags;
        std::optional< expression > init_expr;
        std::vector< expression_arg > init_args;
        std::optional< std::size_t > offset;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_variable_declaration, type, keyword_tags, init_expr, init_args, offset);
    };

    struct ast2_option_default_value
    {
        expression value;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_option_default_value, value);
    };

    struct ast2_option_default_from
    {
        type_symbol symbol;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_option_default_from, symbol);
    };

    using ast2_option_default = rpnx::variant< ast2_option_default_value, ast2_option_default_from >;

    struct ast2_option
    {
        option_kind kind;
        std::optional< ast2_option_default > option_default;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_option, kind, option_default);
    };

    /// A nominal STRUCT declaration and its member declarations.
    struct ast2_struct_declaration
    {
        std::vector< subdeclaroid > declarations;
        std::set< std::string > struct_keywords;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_struct_declaration, declarations, struct_keywords);
    };

    /// A named enum item as written in an ENUM declaration list.
    struct ast2_enum_value_declaration
    {
        std::string name;
        bool is_default = false;
        bool is_null = false;
        std::optional< expression > value;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_enum_value_declaration, name, is_default, is_null, value);
    };

    /// A reserved inclusive numeric range in an ENUM declaration list.
    struct ast2_enum_reserved_range_declaration
    {
        expression from;
        expression to;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_enum_reserved_range_declaration, from, to);
    };

    using ast2_enum_entry = rpnx::variant< ast2_enum_value_declaration, ast2_enum_reserved_range_declaration >;

    /// A nominal ENUM declaration with normalized values computed by enum_info_query.
    struct ast2_enum_declaration
    {
        std::optional< expression > bit_width;
        std::vector< ast2_enum_entry > entries;
        bool allow_unknown = false;
        bool is_ipc = false;
        std::vector< subdeclaroid > declarations;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_enum_declaration, bit_width, entries, allow_unknown, is_ipc, declarations);
    };

    /// A named flag mask as written in a FLAGSET declaration list.
    struct ast2_flagset_value_declaration
    {
        std::string name;
        std::optional< expression > mask;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_flagset_value_declaration, name, mask);
    };

    /// A reserved mask in a FLAGSET declaration list.
    struct ast2_flagset_reserved_declaration
    {
        expression mask;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_flagset_reserved_declaration, mask);
    };

    using ast2_flagset_entry = rpnx::variant< ast2_flagset_value_declaration, ast2_flagset_reserved_declaration >;

    /// A nominal FLAGSET declaration with normalized masks computed by flagset_info_query.
    struct ast2_flagset_declaration
    {
        std::optional< expression > bit_width;
        std::vector< ast2_flagset_entry > entries;
        std::vector< subdeclaroid > declarations;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_flagset_declaration, bit_width, entries, declarations);
    };

    struct ast2_template_declaration
    {
        declared_parameters m_template_args;
        declaroid m_declaroid;
        std::optional< std::int64_t > priority;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_template_declaration, m_template_args, m_declaroid, priority);
    };

    struct ast2_templex
    {
        std::vector< ast2_template_declaration > templates;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_templex, templates);
    };

    struct ast2_function_header
    {
        std::vector< ast2_function_parameter > call_parameters;
        std::optional< std::int64_t > priority;
        std::optional< expression > enable_if;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_function_header, call_parameters, priority, enable_if);
    };

    struct parameters
    {
        std::vector<type_symbol> positional;
        std::map<std::string, type_symbol> named;

        RPNX_MEMBER_METADATA(parameters, positional, named);
    };

    struct ast2_function_definition
    {
        std::optional< type_symbol > return_type;
        std::vector< ast2_function_delegate > delegates;
        function_block body;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_function_definition, return_type, delegates, body);
    };

    struct ast2_functum
    {
        std::vector< ast2_function_definition > functions;
        RPNX_MEMBER_METADATA(ast2_functum, functions);
    };

    struct ast2_function_declaration
    {
        ast2_function_header header;
        ast2_function_definition definition;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_function_declaration, header, definition);
    };

    struct ast2_interface_function_declaration
    {
        std::string name;
        ast2_function_header header;
        ast2_function_definition definition;
        bool has_default_body = false;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_interface_function_declaration, name, header, definition, has_default_body);
    };

    struct ast2_interface_declaration
    {
        std::vector< ast2_interface_function_declaration > functions;
        bool defaultable = false;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_interface_declaration, functions, defaultable);
    };

    struct ast2_implementation_declaration
    {
        type_symbol interface_type;
        std::vector< subdeclaroid > declarations;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_implementation_declaration, interface_type, declarations);
    };

    /**
     * Test declaration parsed from STATIC_TEST, UNIT_TEST, or DUAL_TEST syntax.
     */
    struct ast2_test
    {
        ast2_test_mode mode = ast2_test_mode::static_only;
        static_test_expected_mode expected_mode = static_test_expected_mode::normal;
        ast2_function_definition definition;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_test, mode, expected_mode, definition);
    };

    struct ast2_named_global
    {
        std::string name;
        declaroid declaration;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_named_global, name, declaration);
    };

    struct ast2_named_member
    {
        std::string name;
        declaroid declaration;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_named_member, name, declaration);
    };

    struct ast2_include_if;

    struct [[deprecated("use include_if")]] ast2_include_if
    {
        expression condition;
        ast2_named_declaration declaration;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_include_if, condition, declaration);
    };

    struct ast2_file_declaration
    {
        std::string filename;
        std::map< std::string, std::string > imports;
        std::vector< subdeclaroid > declarations;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_file_declaration, filename, imports, declarations);
    };

    struct ast2_module_declaration
    {
        std::string module_name;
        std::map< std::string, std::string > imports;
        std::vector< subdeclaroid > declarations;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_module_declaration, module_name, imports, declarations);
    };

    struct ast2_declarations
    {
        std::vector< std::pair< std::string, declaroid > > globals;
        std::vector< std::pair< std::string, declaroid > > members;

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

        QUX_AST_METADATA(ast2_extern, lang, symbol, args);
    };

    struct ast2_extern_procedure
    {
        std::string library_name;
        std::string external_symbol_name;
        std::optional< std::string > version;
        bool is_optional = false;
        std::optional< ast2_asm_callable > callable;

        QUX_AST_METADATA(ast2_extern_procedure, library_name, external_symbol_name, version, is_optional, callable);
    };

    std::string to_string(ast2_function_declaration const& ref);
    std::string to_string(declaroid const& ref);
    std::string to_string(expression const& ref);
    std::string to_string(expression const& ref, bool print_locations);

} // namespace quxlang

#endif // AST2_ENTITY_HEADER_GUARD
