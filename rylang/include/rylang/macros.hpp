//
// Created by Ryan Nicholl on 2024-02-04.
//

#ifndef QUX_MACROS_HPP
#define QUX_MACROS_HPP

#include <compare>


#define QUX_GETDEP(dname, what, args) auto dname ## _dep = get_dependency([&]{ return c->lk_ ## what args ; }); if (!ready()) return; auto const & dname = dname ## _dep ->get();
#define QUX_GETDEP_T(dname, what, args, T) auto dname ## _dep = get_dependency([&]{ return c->lk_ ## what args ; }); if (!ready()) return; T dname = dname ## _dep ->get();

#define QUX_CO_GETDEP(dname, what, args) auto dname = co_await *c->lk_ ## what args;

#define QUX_TIECMP(c, x) auto tie() const { return  std::tie x ; } auto tie() const { return std::tie x ; } std::strong_ordering operator <=>(c const & other) { if (tie() < other.tie()) return std::strong_ordering::less; else if (other.tie() < tie()) return std::strong_ordering::greater; return std::strong_ordering::equal; }

#endif //QUX_MACROS_HPP
