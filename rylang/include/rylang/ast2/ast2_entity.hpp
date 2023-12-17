//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef AST2_ENTITY_HEADER_GUARD
#define AST2_ENTITY_HEADER_GUARD

#include <boost/variant.hpp>
#include <rylang/ast2/ast2_function_arg.hpp>

namespace rylang
{
    struct ast2_namespace_declaration;
    struct ast2_variable_declaration;
    struct ast2_class_declaration;
    struct ast2_function_declaration;
    struct ast2_class_template_declaration;
    struct ast2_functum;

    using ast2_declaration = boost::variant< std::monostate, boost::recursive_wrapper< ast2_namespace_declaration >, boost::recursive_wrapper< ast2_variable_declaration >, boost::recursive_wrapper< ast2_class_template_declaration >, boost::recursive_wrapper< ast2_class_declaration >, boost::recursive_wrapper< ast2_function_declaration > >;

    using ast2_map_entity = boost::variant< std::monostate, boost::recursive_wrapper< ast2_functum >, boost::recursive_wrapper< ast2_class_declaration >, boost::recursive_wrapper<ast2_variable_declaration> >;

    struct ast2_functum
    {
        std::vector< ast2_function_declaration > functions;
        std::strong_ordering operator<=>(const ast2_functum& other) const = default;
    };

    struct ast2_namespace_declaration
    {
        std::vector< std::pair< std::string, ast2_declaration > > entities;

        std::strong_ordering operator<=>(const ast2_namespace_declaration& other) const = default;
    };

    struct ast2_variable_declaration
    {
        type_symbol type;
        std::optional< std::size_t > offset;
        std::optional< expression > include_if;
        std::strong_ordering operator<=>(const ast2_variable_declaration& other) const = default;
    };

    struct ast2_class_declaration
    {
        std::vector< std::pair< std::string, ast2_declaration > > members;
        std::vector< std::pair< std::string, ast2_declaration > > globals;

        std::set< std::string > class_keywords;

        std::strong_ordering operator <=>(const ast2_class_declaration& other) const = default;
    };

    struct ast2_class_template_declaration
    {
        std::vector< type_symbol > m_template_args;
        ast2_class_declaration m_class;

        std::strong_ordering operator<=>(const ast2_class_template_declaration& other) const = default;
    };

    struct ast2_function_declaration
    {
        std::vector< ast2_function_arg > args;
        std::optional< type_symbol > return_type;
        std::optional< type_symbol > this_type;
        std::vector< function_delegate > delegates;
        std::optional< std::int64_t > priority;
        function_block body;

        std::strong_ordering operator<=>(const ast2_function_declaration& other) const = default;
    };

} // namespace rylang

#endif // AST2_ENTITY_HEADER_GUARD
