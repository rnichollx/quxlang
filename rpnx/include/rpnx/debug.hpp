//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef QUXLANG_DEBUG_HEADER_GUARD
#define QUXLANG_DEBUG_HEADER_GUARD

#ifndef NDEBUG
#define QUXLANG_DEBUG(x) x
#else
#define QUXLANG_DEBUG(x)
#endif

#endif //DEBUG_HEADER_GUARD
