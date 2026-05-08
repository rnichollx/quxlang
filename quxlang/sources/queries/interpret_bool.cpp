// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/interpret_bool_spec.hpp>
#include "quxlang/parsers/parse_type_symbol.hpp"

#include "quxlang/macros.hpp"

namespace
{
    auto parse_type_symbol_text(std::string const& text) -> quxlang::type_symbol
    {
        auto ctx = quxlang::parsers::make_unlocated_parsing_context(text);
        auto result = quxlang::parsers::parse_type_symbol(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw quxlang::semantic_compilation_error("Input not fully parsed");
        }
        return result;
    }
}

rpnx::querygraph::coroutine< quxlang::interpret_bool_spec > quxlang::interpret_bool_impl(expr_interp_input input)
{
    // Get the general interpret value, check if it's a boolean, and return it.
    // if it's not a boolean, throw an error.
    auto val = co_await rpnx::querygraph::request< interpret_value_query >(input);

    auto booltype = parse_type_symbol_text("BOOL");

    if (val.type != booltype)
    {
        throw quxlang::semantic_compilation_error("Expected boolean value");
    }
    assert(val.data.size() == 1);

    co_return val.data != std::vector< std::byte >{std::byte{0}};
}
