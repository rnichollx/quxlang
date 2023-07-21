//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
#define RPNX_RYANSCRIPT1031_COLLECTOR_HEADER

#include <vector>
#include <string>
#include <iostream>

#include "rylang/ast/class_ast.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/ast/function_arg_ast.hpp"
#include "rylang/ast/member_varaible_ast.hpp"
#include "rylang/ast/symbol_ref_ast.hpp"
#include "rylang/ast/type_ref_ast.hpp"
#include "rylang/ex/unexpected_eof.hpp"
#

namespace rylang
{
  class collector
    {
        struct symbol_frame
        {
        };
        std::vector< std::string > m_symbol_stack;

      private:
        void push_symbol(std::string symbol)
        {
            m_symbol_stack.push_back(std::move(symbol));
        }
        void pop_symbol()
        {
            m_symbol_stack.pop_back();
        }

        template < typename It >
        std::runtime_error unimplemented(It&)
        {
            return std::runtime_error("not implemented");
        }

        template < typename It >
        void expect_more(It& pos, It end)
        {
            if (pos == end)
            {
                throw std::runtime_error("Unexpected end of input text.");
            }
        }

      public:
        std::function< void(std::string const&, function_ast const&) > emit_function;
        std::function< void(std::string const&, class_ast const&) > emit_class;

        template < typename It >
        void collect(It begin, It end)
        {
            auto pos = begin;

            skip_wsc(pos, end);
            if (pos == end)
                return;

            if (skip_symbol_if_is(pos, end, "::"))
            {
                collect_symbol(pos, end);
                return;
            }
            else if (skip_symbol_if_is(pos, end, "#"))
            {
                throw std::runtime_error("Not implemented");
                //  collect_directive(pos, end);
                return;
            }
            else
            {
                throw std::exception();
            }
        }

        // Call this function AFTER :: has been found.
        template < typename It >
        void collect_symbol(It& pos, It end)
        {
            skip_wsc(pos, end);
            expect_more(pos, end);

            std::string remaining = std::string(pos, end);
            std::string name = get_skip_identifier(pos, end);

            if (name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            while (skip_whitespace(pos, end) || skip_line_comment(pos, end))
                ;

            if (skip_keyword_if_is(pos, end, "FUNCTION"))
            {
                function_ast f;
                collect_function(pos, end, f);

                if (emit_function)
                {
                    emit_function(name, f);
                }
            }
            else if (skip_keyword_if_is(pos, end, "CLASS"))
            {
                class_ast c;
                collect_class(pos, end, c);

                if (emit_class)
                {
                    emit_class(name, c);
                }
            }

            skip_wsc(pos, end);

            if (pos == end)
            {
                return;
            }
            else if (skip_symbol_if_is(pos, end, "::"))
            {
                collect_symbol(pos, end);
            }
            else
            {
                throw std::runtime_error("Expected '::' or EOF");
            }
        }

        // Call this function after "FUNCTION",
        template < typename It >
        void collect_function(It& pos, It it_end, function_ast& f)
        {
            while (skip_whitespace(pos, it_end) || skip_line_comment(pos, it_end))
                ;

            if (pos == it_end)
            {
                throw unexpected_eof< It >{pos};
            }

            while (skip_whitespace(pos, it_end) || skip_line_comment(pos, it_end))
                ;

            std::string sym = get_symbol(pos, it_end);

            std::string remaining = std::string(pos, it_end);
            if (!skip_symbol_if_is(pos, it_end, "("))
            {
                throw std::runtime_error("Expected '('");
            }

            collect_function_args(pos, it_end, f);
            collect_function_body(pos, it_end, f);
        }

        template < typename It >
        void collect_class(It& pos, It it_end, class_ast& c)
        {
            skip_wsc(pos, it_end);

            if (pos == it_end)
            {
                throw unexpected_eof< It >{pos};
            }

            skip_wsc(pos, it_end);

            std::string sym = get_symbol(pos, it_end);

            std::string remaining = std::string(pos, it_end);
            if (!skip_symbol_if_is(pos, it_end, "{"))
            {
                throw std::runtime_error("Expected '{'");
            }

            skip_wsc(pos, it_end);

            while (skip_symbol_if_is(pos, it_end, "."))
            {
                collect_class_member(pos, it_end, c);
                skip_wsc(pos, it_end);
            }

            if (!skip_symbol_if_is(pos, it_end, "}"))
            {
                throw std::runtime_error("Expected '}'");
            }
        }

        template < typename It >
        void collect_class_member(It& pos, It it_end, class_ast& c)
        {
            std::cout << "Found class member" << std::endl;
            skip_wsc(pos, it_end);

            expect_more(pos, it_end);

            std::string name = get_skip_identifier(pos, it_end);

            if (name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            skip_wsc(pos, it_end);

            if (skip_keyword_if_is(pos, it_end, "VAR"))
            {
                member_variable_ast member;
                member.name = name;
                skip_wsc(pos, it_end);

                // now a type
                collect_type_symbol(pos, it_end, member.type);

                std::cout << "Found member variable " << member.name << " of type " << member.type.to_string() << std::endl;
                skip_wsc(pos, it_end);
                if (!skip_symbol_if_is(pos, it_end, ";"))
                {
                    std::string remaining = std::string(pos, it_end);
                    throw std::runtime_error("Expected ';'");
                }
            }

            else
            {
                std::string remaining = std::string(pos, it_end);
                throw std::runtime_error("Expected VAR");
            }
        }

        template < typename It >
        void collect_type_symbol(It& pos, It end, type_ref_ast& type_ref)
        {
            skip_wsc(pos, end);

            // TODO: Support all integers
            if (skip_keyword_if_is(pos, end, "I32"))
            {
                type_ref.val.get() = integral_keyword_ast{true, 32};
            }

            else if (get_identifier(pos, end) != "")
            {
                symbol_ref_ast sym;
                collect_lookup_symbol(pos, end, sym);
                type_ref.val.get() = sym;
            }

            else
            {
                throw std::runtime_error("Expected type");
            }
        }

        template < typename It >
        void collect_lookup_symbol(It& pos, It end, symbol_ref_ast& typesymbol)
        {
            while (skip_whitespace(pos, end) || skip_line_comment(pos, end))
                ;

            if (pos == end)
            {
                throw unexpected_eof< It >{pos};
            }

            std::string name = get_skip_identifier(pos, end);

            if (name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            typesymbol.name += name;

            while (skip_whitespace(pos, end) || skip_line_comment(pos, end))
                ;

            if (skip_symbol_if_is(pos, end, "::"))
            {
                typesymbol.name += "::";
                collect_lookup_symbol(pos, end, typesymbol);
                return;
            }

            return;
        }

        template < typename It >
        void collect_function_args(It& pos, It end, function_ast& f)
        {
            skip_wsc(pos, end);
            // expect_more(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                return;
            }

            function_arg_ast arg;

            if (skip_symbol_if_is(pos, end, "@"))
            {
                arg.external_name = get_skip_identifier(pos, end);
                if (arg.external_name.empty())
                {
                    throw std::runtime_error("Expected identifier after '@' ");
                }

                if (!skip_whitespace(pos, end))
                {
                    throw std::runtime_error("Expected whitespace after external parameter name");
                }
            }

            if (!skip_symbol_if_is(pos, end, "%"))
            {
                throw std::runtime_error("Expected '%' or ')'");
            }

            std::string& arg_name = arg.name;
            type_ref_ast& arg_type = arg.type;
            arg_type.val.get() = symbol_ref_ast{};

            // TODO: Collect type ref
            symbol_ref_ast& arg_type_symbol = std::get< symbol_ref_ast >(arg_type.val.get());

            arg_name = get_skip_identifier(pos, end);
            if (arg_name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            skip_wsc(pos, end);

            // TODO: collect_type_symbol
            arg_type.val.get() = symbol_ref_ast{};
            collect_lookup_symbol(pos, end, std::get< symbol_ref_ast >(arg_type.val.get()));

            f.args.push_back(std::move(arg));

            skip_wsc(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                return;
            }
            else if (skip_symbol_if_is(pos, end, ","))
            {
                collect_function_args(pos, end, f);
                return;
            }
            else
            {
                throw std::runtime_error("Expected ',' or ')'");
            }
        }

        template < typename It >
        void collect_function_body(It& pos, It end, function_ast& f)
        {
            skip_wsc(pos, end);

            std::string remaining = std::string(pos, end);

            if (!skip_symbol_if_is(pos, end, "{"))
            {
                throw std::runtime_error("Expected '{'");
            }

            while (skip_whitespace(pos, end) || skip_line_comment(pos, end))
                ;

            if (skip_symbol_if_is(pos, end, "}"))
            {
                // end of function body
                return;
            }

            throw unimplemented(pos);
            // collect_statement(pos, end, f.body);
        }
    };
}

#endif // RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
