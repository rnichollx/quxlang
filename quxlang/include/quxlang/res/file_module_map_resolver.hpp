//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef QUXLANG_FILE_MODULE_MAP_RESOLVER_HEADER_GUARD
#define QUXLANG_FILE_MODULE_MAP_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/file_module_map.hpp"

namespace quxlang
{
  class file_module_map_resolver
  : public rpnx::resolver_base< compiler, file_module_map >
  {
    public:
      using key_type = void;

      inline file_module_map_resolver() {}

      virtual void process(compiler* c);

  };


}

#endif // QUXLANG_FILE_MODULE_MAP_RESOLVER_HEADER_GUARD
