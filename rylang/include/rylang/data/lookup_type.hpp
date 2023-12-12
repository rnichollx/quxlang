//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RYLANG_LOOKUP_TYPE_HEADER_GUARD
#define RYLANG_LOOKUP_TYPE_HEADER_GUARD

namespace rylang
{
    enum class lookup_type { unknown = 0, free, dot, arrow, scope, parameter };

}
#endif // RYLANG_LOOKUP_TYPE_HEADER_GUARD
