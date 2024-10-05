//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef QUXLANG_DATA_FILE_MODULE_MAP_HEADER_GUARD
#define QUXLANG_DATA_FILE_MODULE_MAP_HEADER_GUARD

#include <string>
#include <map>
#include <set>

namespace quxlang
{
   using file_module_map = std::map<std::string, std::set<std::string> >;
}

#endif // QUXLANG_FILE_MODULE_MAP_HEADER_GUARD
