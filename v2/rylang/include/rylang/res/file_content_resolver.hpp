//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FILE_CONTENT_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FILE_CONTENT_RESOLVER_HEADER

#include <string>
#include <vector>

#include "rpnx/graph_solver.hpp"
#include "rylang/compiler_fwd.hpp"

namespace rylang
{
    class file_content_resolver : public rpnx::output_base< compiler, std::string >
    {
      private:
        std::string input_filename;

      public:
        using key_type = std::string;

        file_content_resolver(std::string input_filename) : input_filename(input_filename)
        {
        }

        void process(compiler* c);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FILE_CONTENT_RESOLVER_HEADER
