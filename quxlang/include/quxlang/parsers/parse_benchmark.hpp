// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_BENCHMARK_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_BENCHMARK_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_call_arguments.hpp>
#include <quxlang/parsers/parse_function_args.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/string_literal.hpp>

#include <set>

namespace quxlang::parsers
{
    /** Parses a benchmark declaration without resolving its optional CLI parser. */
    inline auto try_parse_benchmark(parsing_context& ctx) -> std::optional< ast2_benchmark >
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator end = ctx.iter_end;
        parse_iterator begin = pos;
        if (!skip_keyword_if_is(pos, end, "BENCHMARK"))
        {
            return std::nullopt;
        }
        ast2_benchmark result;
        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, "["))
        {
            std::set< std::string > names;
            do
            {
                skip_whitespace_and_comments(pos, end);
                std::string name = parse_identifier(pos, end);
                if (name.empty() || !names.insert(name).second)
                {
                    throw syntax_compilation_error("BENCHMARK requires distinct measurement names");
                }
                result.measurements.push_back(std::move(name));
                skip_whitespace_and_comments(pos, end);
            } while (skip_symbol_if_is(pos, end, ","));
            if (!skip_symbol_if_is(pos, end, "]"))
            {
                throw syntax_compilation_error("Expected ']' after benchmark measurements");
            }
        }
        if (result.measurements.empty())
        {
            result.measurements.emplace_back();
        }
        skip_whitespace_and_comments(pos, end);
        result.parameters = parse_function_args(ctx);
        std::set< std::string > parameter_names;
        for (ast2_function_parameter const& parameter : result.parameters)
        {
            if (!parameter.api_name.has_value() || parameter.is_pack || parameter.is_named_rest || parameter.default_expr.has_value())
            {
                throw syntax_compilation_error("Benchmark parameters require explicit named arguments without defaults or packs");
            }
            if (!parameter_names.insert(*parameter.api_name).second)
            {
                throw syntax_compilation_error("Duplicate benchmark parameter: " + *parameter.api_name);
            }
        }
        std::set< std::string > case_names;
        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            if (skip_keyword_if_is(pos, end, "CLI_PARSER"))
            {
                if (result.cli_parser.has_value())
                {
                    throw syntax_compilation_error("Duplicate CLI_PARSER");
                }
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, "("))
                {
                    throw syntax_compilation_error("Expected '(' after CLI_PARSER");
                }
                result.cli_parser = parse_type_symbol(ctx);
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    throw syntax_compilation_error("Expected ')' after CLI_PARSER type");
                }
            }
            else if (skip_keyword_if_is(pos, end, "CASE"))
            {
                ast2_benchmark_case entry;
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, "("))
                {
                    throw syntax_compilation_error("Expected '(' after CASE");
                }
                skip_whitespace_and_comments(pos, end);
                std::optional< std::string > name = try_parse_string_literal(pos, end);
                if (!name.has_value() || name->empty() || !case_names.insert(*name).second)
                {
                    throw syntax_compilation_error("CASE requires a distinct nonempty string name");
                }
                entry.name = std::move(*name);
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    if (!skip_symbol_if_is(pos, end, ","))
                    {
                        throw syntax_compilation_error("Expected ',' before CASE arguments");
                    }
                    entry.arguments = parse_call_argument_list(ctx, ")");
                }
                std::set< std::string > supplied;
                for (expression_arg const& argument : entry.arguments)
                {
                    if (!argument.name.has_value() || !parameter_names.contains(*argument.name) || !supplied.insert(*argument.name).second)
                    {
                        throw syntax_compilation_error("CASE requires each declared benchmark argument exactly once");
                    }
                }
                if (supplied != parameter_names)
                {
                    throw syntax_compilation_error("CASE is missing benchmark arguments");
                }
                result.cases.push_back(std::move(entry));
            }
            else
            {
                break;
            }
        }
        if (skip_keyword_if_is(pos, end, "ONESHOT"))
        {
            result.oneshot = true;
        }
        else if (!skip_keyword_if_is(pos, end, "BODY"))
        {
            throw syntax_compilation_error("BENCHMARK requires ONESHOT or BODY");
        }
        result.body = parse_function_block(ctx);
        std::size_t loops = 0;
        for (function_statement const& statement : result.body.statements)
        {
            if (statement.type_is< function_benchmark_loop >())
            {
                ++loops;
            }
        }
        if (loops != (result.oneshot ? 0 : 1))
        {
            throw syntax_compilation_error("BODY requires exactly one direct BENCHMARK_LOOP; ONESHOT requires none");
        }
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_BENCHMARK_HEADER_GUARD
