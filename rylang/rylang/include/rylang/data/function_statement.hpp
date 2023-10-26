//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_STATEMENT_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_STATEMENT_HEADER

#include "rylang/data/expression.hpp"
#include <boost/variant.hpp>

namespace rylang
{
    struct function_expression_statement;
    struct function_block;
    struct function_if_statement;

    // TODO: Implement while
    struct function_while_statement
    {
    };
    // TODO: Implement var
    struct function_var_statement
    {
    };

    using function_statement =
        boost::variant< std::monostate, boost::recursive_wrapper< function_block >, boost::recursive_wrapper< function_expression_statement >, boost::recursive_wrapper< function_if_statement >,
                        boost::recursive_wrapper< function_while_statement >, boost::recursive_wrapper< function_var_statement > >;

    struct function_block
    {
        std::vector< function_statement > statements;
    };

} // namespace rylang

#include "rylang/data/function_expression_statement.hpp"
#include "rylang/data/function_if_statement.hpp"

#endif // RPNX_RYANSCRIPT1031_FUNCTION_STATEMENT_HEADER
