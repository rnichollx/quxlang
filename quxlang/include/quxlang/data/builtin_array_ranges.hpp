// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_BUILTIN_ARRAY_RANGES_HEADER_GUARD
#define QUXLANG_DATA_BUILTIN_ARRAY_RANGES_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace quxlang
{
    /**
     * The declaration provides the index range protocol of a canonical built-in array through ordinary struct members.
     * The range carries its bound in its type, and its iterator stores one SZ index.
     */
    inline auto builtin_array_index_declaration(array_type const& array) -> ast2_struct_declaration
    {
        QUXLANG_COMPILER_BUG_IF(!typeis< expression_numeric_literal >(array.element_count), "Array index range requires a canonical element count");
        type_symbol range_type = subsymbol{.of = array, .name = "__INDEX_RANGE"};
        type_symbol iterator_type = subsymbol{.of = range_type, .name = "__ITERATOR"};
        expression index = expression_thisdot_reference{.field_name = "__INDEX"};
        expression other = expression_symbol_reference{.symbol = freebound_identifier{"OTHER"}};
        expression receiver = expression_symbol_reference{.symbol = freebound_identifier{"THIS"}};
        ast2_function_parameter constant_iterator_receiver{.name = "THIS", .api_name = "THIS", .type = make_cref(iterator_type)};
        ast2_function_parameter mutable_iterator_receiver{.name = "THIS", .api_name = "THIS", .type = make_mref(iterator_type)};

        auto append_function = [](ast2_struct_declaration& owner, std::string name, std::vector< ast2_function_parameter > parameters, type_symbol return_type, std::vector< function_statement > statements)
        {
            ast2_function_declaration function;
            function.header.call_parameters = std::move(parameters);
            function.definition.return_type = std::move(return_type);
            function.definition.body.statements = std::move(statements);
            owner.declarations.push_back(member_subdeclaration{.decl = std::move(function), .name = std::move(name)});
        };

        ast2_struct_declaration iterator;
        iterator.declarations.push_back(member_subdeclaration{.decl = ast2_variable_declaration{.type = size_type{}}, .name = "__INDEX"});
        append_function(iterator, "CONSTRUCTOR",
            {{.name = "INDEX", .api_name = "INDEX", .type = size_type{}}}, void_type{},
            {function_expression_statement{.expr = expression_binary{.operator_str = ":=", .lhs = index,
                .rhs = expression_symbol_reference{.symbol = freebound_identifier{"INDEX"}}}}});
        for (std::string const& operation : {std::string("=="), std::string("<=>")})
        {
            append_function(iterator, "OPERATOR" + operation,
                {constant_iterator_receiver, {.name = "OTHER", .api_name = "OTHER", .type = make_cref(iterator_type)}},
                operation == "==" ? type_symbol(bool_type{}) : type_symbol(builtin_symbol{"ORDER"}),
                {function_return_statement{.expr = expression_binary{.operator_str = operation, .lhs = index,
                    .rhs = expression_dotreference{.lhs = other, .field_name = "__INDEX"}}}});
        }
        append_function(iterator, "OPERATOR++", {mutable_iterator_receiver}, iterator_type,
            {function_var_statement{.name = "__PREVIOUS", .type = iterator_type, .equals_initializer = receiver},
             function_expression_statement{.expr = expression_unary_postfix{.operator_str = "++", .lhs = index}},
             function_return_statement{.expr = expression_symbol_reference{.symbol = freebound_identifier{"__PREVIOUS"}}}});
        append_function(iterator, "OPERATOR+=",
            {mutable_iterator_receiver, {.name = "OTHER", .api_name = "OTHER", .type = size_type{}}}, make_mref(iterator_type),
            {function_expression_statement{.expr = expression_binary{.operator_str = "+=", .lhs = index, .rhs = other}},
             function_return_statement{.expr = receiver}});
        append_function(iterator, "OPERATOR->", {constant_iterator_receiver}, size_type{},
            {function_return_statement{.expr = index}});

        ast2_struct_declaration range;
        range.declarations.push_back(global_subdeclaration{.decl = std::move(iterator), .name = "__ITERATOR"});
        for (std::string const& boundary : {std::string("BEGIN"), std::string("END")})
        {
            append_function(range, boundary,
                {{.name = "THIS", .api_name = "THIS", .type = make_cref(range_type)}}, iterator_type,
                {function_return_statement{.expr = expression_call{
                    .callee = expression_symbol_reference{.symbol = iterator_type},
                    .args = {{.name = "INDEX", .value = boundary == "BEGIN" ? expression(expression_numeric_literal{.value = "0"}) : array.element_count}},
                }}});
        }

        ast2_struct_declaration declaration;
        declaration.declarations.push_back(global_subdeclaration{.decl = std::move(range), .name = "__INDEX_RANGE"});
        append_function(declaration, "INDEXES",
            {{.name = "THIS", .api_name = "THIS", .type = ptrref_type{.target = array, .ptr_class = pointer_class::ref, .qual = qualifier::auto_, .is_ibc = std::nullopt}}}, range_type,
            {function_return_statement{.expr = expression_call{.callee = expression_symbol_reference{.symbol = range_type}}}});
        return declaration;
    }
}

#endif // QUXLANG_DATA_BUILTIN_ARRAY_RANGES_HEADER_GUARD
