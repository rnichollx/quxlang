//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_FILE_MODULE_MAP_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FILE_MODULE_MAP_RESOLVER_HEADER

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

#endif // RPNX_RYANSCRIPT1031_FILE_MODULE_MAP_RESOLVER_HEADER
