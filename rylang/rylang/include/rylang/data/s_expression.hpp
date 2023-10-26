//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_S_EXPRESSION_HEADER
#define RPNX_RYANSCRIPT1031_S_EXPRESSION_HEADER

#include <string>
#include <boost/variant.hpp>

namespace rylang
{

    struct s_expression_add;
    struct s_expression_addp;
    struct s_expression_addw;
    struct s_expression_call;
    struct s_expression_lvalue_reference;

    struct s_expression_this_reference
    {
    };

    struct s_expression_thisdot_reference
    {
        std::string field_name;
    };

    struct s_expression_lvalue_reference
    {
    };

    using s_expression = boost::variant< std::monostate, boost::recursive_wrapper< s_expression_add >, boost::recursive_wrapper< s_expression_add >, boost::recursive_wrapper< s_expression_addw >,
                                       boost::recursive_wrapper< s_expression_call >, boost::recursive_wrapper< s_expression_lvalue_reference >, s_expression_this_reference, s_expression_thisdot_reference >;

    using s_expression_ptr = std::shared_ptr<s_expression>;
}

#endif // RPNX_RYANSCRIPT1031_S_EXPRESSION_HEADER
