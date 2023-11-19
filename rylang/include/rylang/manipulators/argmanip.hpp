//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_ARGMANIP_HEADER
#define RPNX_RYANSCRIPT1031_ARGMANIP_HEADER

#include "rylang/ast/function_arg_ast.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/data/call_parameter_information.hpp"

namespace rylang
{
    inline call_parameter_information to_call_overload_set(std::vector< function_arg_ast > const & args)
    {
        // TODO: For now, assume all function argument types are non-contextual
        //  Later, we can add support for context conversion

        call_parameter_information result;

        for (auto& arg : args)
        {
            result.argument_types.push_back(arg.type);
        }

        return result;
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ARGMANIP_HEADER
