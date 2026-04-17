// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_FWD_HEADER_GUARD
#define QUXLANG_PARSERS_FWD_HEADER_GUARD

#include <optional>

namespace quxlang::parsers
{
    struct parsing_context;

    std::optional< function_block > try_parse_function_block(parsing_context& ctx);
    function_block parse_function_block(parsing_context& ctx);

}

#endif // FWD_HPP
