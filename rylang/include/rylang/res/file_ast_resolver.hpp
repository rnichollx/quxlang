//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_FILE_AST_RESOLVER_HEADER_GUARD
#define RYLANG_FILE_AST_RESOLVER_HEADER_GUARD

#include <string>
#include <vector>

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/compiler_fwd.hpp"

namespace rylang
{
    class file_ast_resolver : public rpnx::resolver_base< compiler, ast2_file_declaration >
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

#endif // RYLANG_FILE_SST_RESOLVER_HEADER_GUARD
