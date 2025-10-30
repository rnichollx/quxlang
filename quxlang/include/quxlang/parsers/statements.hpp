// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_STATEMENTS_HEADER_GUARD
#define QUXLANG_PARSERS_STATEMENTS_HEADER_GUARD

#include <quxlang/data/statements.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional<function_place_statement> try_parse_place_statement(It& pos, It end)
    {
        // there are three forms,
        // PLACE AT(loc) type :(args...);
        // PLACE AT(loc) type := assign_init_expr;
        // PLACE AT(loc) type;



    }
} // namespace quxlang::parsers

#endif // QUXLANG_STATEMENT_HPP
