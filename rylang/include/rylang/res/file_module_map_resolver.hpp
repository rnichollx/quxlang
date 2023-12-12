//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef RYLANG_FILE_MODULE_MAP_RESOLVER_HEADER_GUARD
#define RYLANG_FILE_MODULE_MAP_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/file_module_map.hpp"

namespace rylang
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

#endif // RYLANG_FILE_MODULE_MAP_RESOLVER_HEADER_GUARD
