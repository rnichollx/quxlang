//
// Created by Ryan Nicholl on 3/9/23.
//

#ifndef RPNX_RYANSCRIPT1031_MAP_ALG_HEADER
#define RPNX_RYANSCRIPT1031_MAP_ALG_HEADER

#include <map>
namespace rs1031
{
    template < typename K, typename V, typename F >
    auto access_or_create(std::map< K, V >& map, K const& key, F&& f) -> V&
    {
        auto it = map.find(key);
        if (it == map.end())
        {
            it = map.insert({key, f()}).first;
        }
        return it->second;
    }
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_MAP_ALG_HEADER
