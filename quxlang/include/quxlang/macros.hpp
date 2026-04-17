// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MACROS_HEADER_GUARD
#define QUXLANG_MACROS_HEADER_GUARD

#include <compare>
#include <optional>
#include <tuple>
#include <utility>

#include <rpnx/macros.hpp>
#include <string>

#include "quxlang/ast2/source_location.hpp"
#include "quxlang/exception.hpp"

#include "rpnx/unimplemented.hpp"
// clang-format off

// MOVEREL is Move In Release Configuration
// Helps preserve objects for debugging in debug builds.
#ifdef NDEBUG
#define MOVEREL(x) std::move(x)
#else
#define MOVEREL(x) x
#endif




// QUXLANG_WITH_SOURCE_LOCATION_METADATA adds source provenance to syntax/IR nodes
// and includes it in the node metadata.

#define QUXLANG_WITH_SOURCE_LOCATION_METADATA(ty, ...) \
    std::optional< quxlang::source_location > location; \
    RPNX_MEMBER_METADATA(ty, location, __VA_ARGS__)

#define QUXLANG_WITH_SOURCE_LOCATION_EMPTY_METADATA(ty) \
    std::optional< quxlang::source_location > location; \
    RPNX_MEMBER_METADATA(ty, location)

// Compatibility alias for older AST declarations.
#define QUX_AST_METADATA(ty, ...) QUXLANG_WITH_SOURCE_LOCATION_METADATA(ty, __VA_ARGS__)

#define QUX_WHY( strs )

#define QUX_TIECMP(c, x) auto tie() const { return  std::tie x ; } auto tie() const { return std::tie x ; } std::strong_ordering operator <=>(c const & other) { if (tie() < other.tie()) return std::strong_ordering::less; else if (other.tie() < tie()) return std::strong_ordering::greater; return std::strong_ordering::equal; }


#define QUXLANG_UNREACHABLE() __builtin_unreachable()



#ifdef QUXLANG_ASSUME_BUGS_UNREACHABLE
#define QUXLANG_COMPILER_BUG(x) QUXLANG_UNREACHABLE();
#else
#define QUXLANG_COMPILER_BUG(x) throw quxlang::compiler_bug(x);
#endif


#ifndef NDEBUG
#define QUXLANG_COMPILER_BUG_IF(x, y) if (x) QUXLANG_COMPILER_BUG(y)
#else
#define QUXLANG_COMPILER_BUG_IF(x, y)
#endif

#ifndef NDEBUG
#define QUXLANG_DEBUG_VALUE(x) [[maybe_unused]] auto quxlang_dbg_val_ ## __LINE__ = x;
#define QUXLANG_DEBUG_NAMED_VALUE(name, x) [[maybe_unused]] auto name = x;
#define QUXLANG_ASSERT(x) if (!(x)) throw quxlang::assert_failure(#x);


#else
#define QUXLANG_DEBUG_VALUE(x)
#define QUXLANG_DEBUG_NAMED_VALUE(name, x)
#define QUXLANG_ASSERT(x)
#endif


#ifndef NDEBUG
#define QUXLANG_DEBUG(x) x
#define QUXLANG_IN_DEBUG true
#else
#define QUXLANG_DEBUG(x)
#define QUXLANG_IN_DEBUG false
#endif


#ifndef QUXLANG_DEBUG_MESSAGES_ENABLED
#ifdef NDEBUG
#define QUXLANG_DEBUG_MESSAGES_ENABLED false
#else
#define QUXLANG_DEBUG_MESSAGES_ENABLED true
#endif
#endif





// clang-format on

#endif // QUX_MACROS_HPP
