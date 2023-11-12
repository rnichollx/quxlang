//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FILE_AST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FILE_AST_RESOLVER_HEADER

#include <string>
#include <vector>

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/compiler_fwd.hpp"

namespace rylang
{
    class file_ast_resolver : public rpnx::resolver_base< compiler, file_ast >
    {
      private:
        std::string input_filename;

      public:
        using key_type = std::string;

        file_ast_resolver(std::string input_filename) : input_filename(input_filename)
        {
        }

        void process(compiler* c);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FILE_SST_RESOLVER_HEADER
