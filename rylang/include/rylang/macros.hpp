//
// Created by Ryan Nicholl on 2024-02-04.
//

#ifndef QUX_MACROS_HPP
#define QUX_MACROS_HPP

#include <compare>


#define QUX_RESOLVER(name, input, output) \
class name ## _resolver : public rpnx::resolver_base< compiler, output > { \
private:                                   \
  input m_input;                          \
input const & get_input() const { return m_input; } \
using input_type = input; \
name ## _resolver(in input) : m_input(in) {} \
virtual void process(compiler * c) override; \
};

#define QUX_CO_RESOLVER(name, input, output) \
class name ## _resolver : public rpnx::co_resolver_base< compiler, input, output > { \
 public: \
name ## _resolver(in input_type) : resolver_base< output >(std::move(in)) {}                                    \
 virtual void process(compiler * c) override;                                                              \
};

#define QUX_RESOLVER_IMPL_FUNC_DEF(nameV) \
void rylang::nameV ## _resolver::process(compiler * c) \



#define QUX_GETDEP(dname, what, args) auto dname ## _dep = get_dependency([&]{ return c->lk_ ## what args ; }); if (!ready()) return; auto const & dname = dname ## _dep ->get();
#define QUX_GETDEP_T(dname, what, args, T) auto dname ## _dep = get_dependency([&]{ return c->lk_ ## what args ; }); if (!ready()) return; T dname = dname ## _dep ->get();

#define QUX_CO_GETDEP(dname, what, args) auto dname = co_await *c->lk_ ## what args;

#define QUX_TIECMP(c, x) auto tie() const { return  std::tie x ; } auto tie() const { return std::tie x ; } std::strong_ordering operator <=>(c const & other) { if (tie() < other.tie()) return std::strong_ordering::less; else if (other.tie() < tie()) return std::strong_ordering::greater; return std::strong_ordering::equal; }

#endif //QUX_MACROS_HPP
