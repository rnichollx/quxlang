// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MACROS_HEADER_GUARD
#define QUXLANG_MACROS_HEADER_GUARD

#include <compare>

#include "quxlang/compiler_fwd.hpp"
#include "rpnx/metadata.hpp"
#include "rpnx/resolver_utilities.hpp"
#include <string>

#include "quxlang/exception.hpp"

#include "rpnx/value.hpp"
// clang-format off

// MOVEREL is Move In Release Configuration
// Helps preserve objects for debugging in debug builds.
#ifdef _NDEBUG
#define MOVEREL(x) std::move(x)
#else
#define MOVEREL(x) x
#endif


// QUX_AST_METADATA is a macro that allows ast2_ X to be converted to X,
// and provides tie functions for comparison, assignment, etc.
#define QUX_AST_METADATA(ty, ...) \
    ast2_source_location location; \
\
    auto tie() const { return std::tie(location, __VA_ARGS__); } \
        auto tie() { return std::tie(location, __VA_ARGS__); }   \
        auto tie_ast() const { return std::tie(__VA_ARGS__); } \
        auto tie_ast() { return std::tie(__VA_ARGS__); } \
        auto serialize_interface() const { return tie(); } \
        auto deserialize_interface() { return tie(); } \
        auto operator<=>(ast2_ ## ty  const& other) const { return rpnx::compare(tie(), other.tie()); } \
        bool operator==(ast2_ ## ty const& other) const { return tie() == other.tie(); } \
        bool operator!=(ast2_ ## ty const& other) const { return tie() != other.tie(); } \
        static auto constexpr strings()  { std::string str = "location," #__VA_ARGS__; std::string s; std::vector<std::string> result{ }; \
        for (char c: str) { if (c == ',') { if (!s.empty()) { result.push_back(std::move(s)); } s.clear(); } else if (c != ' ') { s.push_back(c); } } if (!s.empty()) result.push_back(std::move(s)); return result; } \
        operator ty () const { \
          ty result; \
          result.tie() = this->tie_ast(); \
          return result; }

#define QUX_AST_METADATA_NOCONV(ty, ...) \
    ast2_source_location location; \
\
    auto tie() const { return std::tie(location, __VA_ARGS__); } \
        auto tie() { return std::tie(location, __VA_ARGS__); }   \
        auto tie_ast() const { return std::tie(__VA_ARGS__); } \
        auto tie_ast() { return std::tie(__VA_ARGS__); } \
auto serialize_interface() const { return tie(); } \
auto deserialize_interface() { return tie(); } \
        auto operator<=>(ast2_ ## ty  const& other) const { return rpnx::compare(tie(), other.tie()); } \
        bool operator==(ast2_ ## ty const& other) const { return tie() == other.tie(); } \
        bool operator!=(ast2_ ## ty const& other) const { return tie() != other.tie(); } \
        static auto constexpr strings()  { std::string str = "location," #__VA_ARGS__; std::string s; std::vector<std::string> result{ }; \
        for (char c: str) { if (c == ',') { if (!s.empty()) { result.push_back(std::move(s)); } s.clear(); } else if (c != ' ') { s.push_back(c); } } if (!s.empty()) result.push_back(std::move(s)); return result; }



#define QUX_RESOLVER(name, input, output) \
class name ## _resolver : public rpnx::resolver_base< compiler, output > { \
private:                                   \
  input m_input;                          \
input const & get_input() const { return m_input; } \
using input_type = input; \
name ## _resolver(input in ) : m_input(in) {} \
virtual void process(compiler * c) override; \
};


/// This implements a coroutine resolver declaration, to be used in overload files.
// it takes exaclty three arguments: the name of the resolver, the input type, and the output type, in that ordering.
// Example usage: QUX_CO_RESOLVER(foo, input_type, output_type)
// Wrong: QUX_CO_RESOLVER(foo_resolver, input_type, output_type)
//   - This would create a type called foo_resolver_resolver, which is not what we want.
#define QUX_CO_RESOLVER(nameV, inputT, outputT) \
class nameV ## _resolver : public rpnx::co_resolver_base< compiler, outputT, inputT > { \
 public: \
explicit nameV ## _resolver( input_type in) : rpnx::co_resolver_base< compiler, output_type, input_type >(std::move(in)) {}                                    \
rpnx::resolver_coroutine< compiler, output_type > co_process(compiler* c, input_type arg_input) override; \
};

#define QUX_RESOLVER_IMPL_FUNC_DEF(nameV) \
void quxlang::nameV ## _resolver::process(compiler * c) \

/// QUX_CO_RESOLVER_IMPL_FUNC_DEF implements a resolver coroutine definition, inside C++ files.
/// It only requires the name of the resolver, without the _resolver suffix.
/// the other information is provided by the header file.
/// Example usage: QUX_CO_RESOLVER_IMPL_FUNC_DEF(foo) { ... }
/// Wrong: QUX_CO_RESOLVER_IMPL_FUNC_DEF(foo_resolver) { ... }
//   - This would attempt to implement foo_resolver_resolver, which is not what we want.
#define QUX_CO_RESOLVER_IMPL_FUNC_DEF(nameV) \
quxlang::nameV ## _resolver::co_type quxlang::nameV ## _resolver::co_process(compiler* c, input_type arg_input)

#define QUX_WHY( strs )


#define QUX_SUBCO_CLASS_BEGIN(nameV) \
class nameV {                  \
 compiler * c;                 \
  public: \
nameV (compiler * c) : c(c) {}

#define QUX_SUBCO_CLASS_END() };



/// When we need to create objects that run sub-coroutines, we can use
// this is to declare them.
#define QUX_SUBCO_MEMBER_FUNC(nameV, retT, argsV) \
 rpnx::general_coroutine< compiler, retT > nameV argsV

/// Defines a coroutine member
#define QUX_SUBCO_MEMBER_FUNC_DEF(classN, nameV, retT, argsV) \
rpnx::general_coroutine< quxlang::compiler, retT> quxlang::classN::nameV argsV


#define QUX_SUBCO_MEMBER_FUNC_DEF2(classNamespace, className, nameV, retT, argsV) \
rpnx::general_coroutine< quxlang::compiler, retT> classNamespace :: className ::nameV argsV

#define QUX_CO_ANSWER(x) co_return x;

#define QUX_GETDEP(retnameV, Q, args) auto retnameV ## _dep = get_dependency([&]{ return c->lk_ ## Q args ; }); if (!ready()) return; auto const & retnameV = retnameV ## _dep ->get();
#define QUX_GETDEP_T(retnameV, Q, args, T) auto retnameV ## _dep = get_dependency([&]{ return c->lk_ ## Q args ; }); if (!ready()) return; T retnameV = retnameV ## _dep ->get();

#define QUX_CO_DEP(depname, args) *c->lk_ ## depname args

#define QUX_CO_ASK(depname, args) (co_await *c->lk_ ## depname args )

#define QUX_CO_GETDEP(retname, depname, args) auto retname = co_await *c->lk_ ## depname args;

#define QUX_TIECMP(c, x) auto tie() const { return  std::tie x ; } auto tie() const { return std::tie x ; } std::strong_ordering operator <=>(c const & other) { if (tie() < other.tie()) return std::strong_ordering::less; else if (other.tie() < tie()) return std::strong_ordering::greater; return std::strong_ordering::equal; }


#define QUXLANG_UNREACHABLE() __builtin_unreachable()



#ifdef QUXLANG_ASSUME_BUGS_UNREACHABLE
#define QUXLANG_COMPILER_BUG(x) QUXLANG_UNREACHABLE();
#else
#define QUXLANG_COMPILER_BUG(x) throw quxlang::compiler_bug(x);
#endif


#ifndef _NDEBUG
#define QUXLANG_COMPILER_BUG_IF(x, y) if (x) QUXLANG_COMPILER_BUG(y)
#else
#define QUXLANG_COMPILER_BUG_IF(x, y)
#endif

#ifndef _NDEBUG
#define QUXLANG_DEBUG_VALUE(x) auto quxlang_dbg_val_ ## __LINE__ = x;
#define QUXLANG_DEBUG_NAMED_VALUE(name, x) auto name = x;
#define QUXLANG_ASSERT(x) if (!(x)) throw quxlang::assert_failure(#x);


#else
#define QUXLANG_DEBUG_VALUE(x)
#define QUXLANG_DEBUG_NAMED_VALUE(name, x)
#define QUXLANG_ASSERT(x)
#endif






// clang-format on

#endif // QUX_MACROS_HPP

/* Conversion guide:

 Code using QUX_RESOLVER, of format:

  auto foo_dp = get_dependency([&]{ return c->lk_bar(args); });
  if (!ready()) return;
  auto const & foo = foo_dp->get();

 Is replaced in functions using QUX_CO_RESOLVER/QUX_CO_RESOLVER_IMPL_FUNC_DEF using:

 auto foo = co_await QUX_CO_DEP(bar, (args));

*/