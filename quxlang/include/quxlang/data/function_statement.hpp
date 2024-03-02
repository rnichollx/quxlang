//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD
#define QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include "quxlang/data/function_return_statement.hpp"
#include <boost/variant.hpp>
#include <tuple>
#include <utility>

namespace quxlang
{
    struct function_expression_statement;
    struct function_block;
    struct function_if_statement;

    // TODO: Implement while
    struct function_while_statement;

    struct function_var_statement
    {
        std::string name;
        type_symbol type;
        // TODO: support named initializers
        std::vector<expression> initializers;
        std::strong_ordering operator<=>(const function_var_statement& other) const = default;
    };

    using function_statement =
        boost::variant< std::monostate, boost::recursive_wrapper< function_block >, boost::recursive_wrapper< function_expression_statement >, boost::recursive_wrapper< function_if_statement >,
                        boost::recursive_wrapper< function_while_statement >, boost::recursive_wrapper< function_var_statement >, function_return_statement >;



    struct function_block
    {
        std::vector< function_statement > statements;

        auto tie() const
        {
            return std::tie(statements);
        }



        std::strong_ordering operator<=>(const function_block& other) const = default;
    };

} // namespace quxlang

#include "quxlang/data/function_expression_statement.hpp"
#include "quxlang/data/function_if_statement.hpp"
#include "quxlang/data/function_while_statement.hpp"

#endif // QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD
