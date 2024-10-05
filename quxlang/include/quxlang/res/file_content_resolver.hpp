//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_RES_FILE_CONTENT_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_FILE_CONTENT_RESOLVER_HEADER_GUARD

#include <string>
#include <vector>

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"

namespace quxlang
{
    class file_content_resolver : public rpnx::resolver_base< compiler, std::string >
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

} // namespace quxlang

#endif // QUXLANG_FILE_CONTENT_RESOLVER_HEADER_GUARD
