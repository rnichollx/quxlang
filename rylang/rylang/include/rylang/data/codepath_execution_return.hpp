//
// Created by Ryan Nicholl on 10/28/23.
//

#ifndef RPNX_RYANSCRIPT1031_CODEPATH_EXECUTION_RETURN_HEADER
#define RPNX_RYANSCRIPT1031_CODEPATH_EXECUTION_RETURN_HEADER

#include "canonical_type_reference.hpp"
#include <optional>
namespace rylang
{
    struct codepath_execution_return
    {
        canonical_type_reference return_type;
        std::optional<codepath_expression> return_expression;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CODEPATH_EXECUTION_RETURN_HEADER
