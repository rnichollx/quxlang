//
// Created by Ryan Nicholl on 11/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_FRAME_INFORMATION_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_FRAME_INFORMATION_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_overload_set.hpp"
#include "rylang/data/function_frame_information.hpp"
#include "rylang/data/function_if_statement.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    class function_frame_information_resolver : public rpnx::resolver_base< compiler, function_frame_information >
    {
        qualified_symbol_reference m_input;

      public:
        using key_type = qualified_symbol_reference;

        function_frame_information_resolver(key_type input)
            : m_input(input)
        {
          assert(false);
        }

        virtual void process(compiler* c);

      private:
        bool recurse(compiler* c, function_frame_information& frame, function_if_statement statement);
        bool recurse(compiler* c, function_frame_information& frame, function_while_statement statement);
        bool recurse(rylang::compiler* c, rylang::function_frame_information& frame, rylang::function_block const& block);
        bool recurse(rylang::compiler* c, rylang::function_frame_information& frame, rylang::function_var_statement const& statement);
        bool recurse(rylang::compiler*c, rylang::function_frame_information& frame, rylang::function_return_statement const& statement);
        bool recurse(rylang::compiler* c, rylang::function_frame_information& frame, rylang::function_expression_statement const& statement);


    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_FRAME_INFORMATION_RESOLVER_HEADER
