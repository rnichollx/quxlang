//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_FILE_AST_RESOLVER_HEADER_GUARD
#define QUXLANG_FILE_AST_RESOLVER_HEADER_GUARD

#include <string>
#include <vector>

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/ast/file_ast.hpp"
#include "quxlang/compiler_fwd.hpp"

namespace quxlang
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

} // namespace quxlang

#endif // QUXLANG_FILE_SST_RESOLVER_HEADER_GUARD
