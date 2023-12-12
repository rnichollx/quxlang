//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef RYLANG_DEBUG_HEADER_GUARD
#define RYLANG_DEBUG_HEADER_GUARD

#ifndef NDEBUG
#define RYLANG_DEBUG(x) x
#else
#define RYLANG_DEBUG(x)
#endif

#endif //DEBUG_HEADER_GUARD
