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
#include <variant>
#include <utility>
#include "rpnx/variant.hpp"

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
        std::strong_ordering operator<=>(const function_var_statement& other) const
        {
            return rpnx::compare(name, other.name, type, other.type, initializers, other.initializers);
        }
    };

    using function_statement =
        rpnx::variant< std::monostate,  function_block ,  function_expression_statement ,  function_if_statement ,
                         function_while_statement   ,  function_var_statement , function_return_statement >;



    struct function_block
    {
        std::vector< function_statement > statements;

        auto tie() const
        {
            return std::tie(statements);
        }



        auto operator<=>(const function_block& other) const
        {
            return tie() <=> other.tie();
        }
    };

} // namespace quxlang

#include "quxlang/data/function_expression_statement.hpp"
#include "quxlang/data/function_if_statement.hpp"
#include "quxlang/data/function_while_statement.hpp"

#endif // QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD
