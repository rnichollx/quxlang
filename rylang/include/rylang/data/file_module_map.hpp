//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef RYLANG_FILE_MODULE_MAP_HEADER_GUARD
#define RYLANG_FILE_MODULE_MAP_HEADER_GUARD

#include <string>
#include <map>
#include <set>

namespace rylang
{
   using file_module_map = std::map<std::string, std::set<std::string> >;
}

#endif // RYLANG_FILE_MODULE_MAP_HEADER_GUARD
