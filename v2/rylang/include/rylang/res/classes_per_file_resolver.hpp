//
// Created by Ryan Nicholl on 7/21/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASSES_PER_FILE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASSES_PER_FILE_RESOLVER_HEADER
#include "rpnx/graph_solver.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/class_list.hpp"
namespace rylang
{

    class classes_per_file_resolver : public rpnx::output_base< compiler, class_list >
    {
        private:
          std::string input_filename;

        public:
          using key_type = std::string;

          classes_per_file_resolver(std::string input_filename) : input_filename(input_filename)
          {
          }

          void process(compiler* c);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASSES_PER_FILE_RESOLVER_HEADER
