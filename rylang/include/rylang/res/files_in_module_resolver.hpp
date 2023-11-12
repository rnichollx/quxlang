//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_FILES_IN_MODULE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FILES_IN_MODULE_RESOLVER_HEADER

#include "rylang/filelist.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/fwd.hpp"

namespace rylang
{

    class files_in_module_resolver : public rpnx::resolver_base< compiler, filelist >
    {
      public:
        using key_type = std::string;

        inline  files_in_module_resolver(std::string module_id)
            : m_id(module_id)
        {
        }

        virtual void process(compiler* c);

      private:
        std::string m_id;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FILES_IN_MODULE_RESOLVER_HEADER
