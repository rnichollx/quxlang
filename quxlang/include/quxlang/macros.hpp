//
// Created by Ryan Nicholl on 2024-02-04.
//

#ifndef QUX_MACROS_HPP
#define QUX_MACROS_HPP

#include <compare>

#include "quxlang/compiler_fwd.hpp"
#include "rpnx/resolver_utilities.hpp"
#include <string>

// clang-format off


// QUX_AST_METADATA is a macro that allows ast2_ X to be converted to X,
// and provides tie functions for comparison, assignment, etc.
#define QUX_AST_METADATA(ty, ...) \
    ast2_source_location location; \
\
    auto tie() const { return std::tie(location, __VA_ARGS__); } \
        auto tie() { return std::tie(location, __VA_ARGS__); }   \
        auto tie_ast() const { return std::tie(__VA_ARGS__); } \
        auto tie_ast() { return std::tie(__VA_ARGS__); } \
        auto serial_interface() const { return tie(); } \
        auto serial_interface() { return tie(); } \
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
        auto serial_interface() const { return tie(); } \
        auto serial_interface() { return tie(); } \
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


/// This implements a coroutine resolver declaration, to be used in header files.
#define QUX_CO_RESOLVER(nameV, inputT, outputT) \
class nameV ## _resolver : public rpnx::co_resolver_base< compiler, outputT, inputT > { \
 public: \
nameV ## _resolver( input_type in) : rpnx::co_resolver_base< compiler, output_type, input_type >(std::move(in)) {}                                    \
rpnx::resolver_coroutine< compiler, output_type > co_process(compiler* c, input_type arg_input) override; \
};

#define QUX_RESOLVER_IMPL_FUNC_DEF(nameV) \
void quxlang::nameV ## _resolver::process(compiler * c) \

/// This implements a resolver coroutine, inside C++ files.
#define QUX_CO_RESOLVER_IMPL_FUNC_DEF(nameV) \
quxlang::nameV ## _resolver::co_type quxlang::nameV ## _resolver::co_process(compiler* c, input_type arg_input)


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


#define QUX_CO_ANSWER(x) co_return x;

#define QUX_GETDEP(retnameV, Q, args) auto retnameV ## _dep = get_dependency([&]{ return c->lk_ ## Q args ; }); if (!ready()) return; auto const & retnameV = retnameV ## _dep ->get();
#define QUX_GETDEP_T(retnameV, Q, args, T) auto retnameV ## _dep = get_dependency([&]{ return c->lk_ ## Q args ; }); if (!ready()) return; T retnameV = retnameV ## _dep ->get();

#define QUX_CO_DEP(depname, args) *c->lk_ ## depname args;

#define QUX_CO_GETDEP(retname, depname, args) auto retname = co_await *c->lk_ ## depname args;

#define QUX_TIECMP(c, x) auto tie() const { return  std::tie x ; } auto tie() const { return std::tie x ; } std::strong_ordering operator <=>(c const & other) { if (tie() < other.tie()) return std::strong_ordering::less; else if (other.tie() < tie()) return std::strong_ordering::greater; return std::strong_ordering::equal; }


// clang-format on

#endif //QUX_MACROS_HPP
