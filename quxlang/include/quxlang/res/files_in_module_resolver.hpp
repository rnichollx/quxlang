//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef QUXLANG_FILES_IN_MODULE_RESOLVER_HEADER_GUARD
#define QUXLANG_FILES_IN_MODULE_RESOLVER_HEADER_GUARD

#include "quxlang/filelist.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/fwd.hpp"

namespace quxlang
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
} // namespace quxlang

#endif // QUXLANG_FILES_IN_MODULE_RESOLVER_HEADER_GUARD
