//
// Created by Ryan Nicholl on 2/28/23.
//

#ifndef RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
#define RPNX_RYANSCRIPT1031_COLLECTOR_HEADER

#include "ast.hpp"
#include "parser.hpp"
#include <vector>

namespace rs1031
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
        std::function< void(std::string const&, ast_function const&) > emit_function;
        std::function< void(std::string const&, ast_class const&) > emit_class;

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
                ast_function f;
                collect_function(pos, end, f);

                if (emit_function)
                {
                    emit_function(name, f);
                }
            }
            else if (skip_keyword_if_is(pos, end, "CLASS"))
            {
                ast_class c;
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
        void collect_function(It& pos, It it_end, ast_function& f)
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
        void collect_class(It& pos, It it_end, ast_class& c)
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
        void collect_class_member(It& pos, It it_end, ast_class& c)
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
                ast_member_variable member;
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
        void collect_type_symbol(It& pos, It end, ast_type_ref& type_ref)
        {
            skip_wsc(pos, end);

            // TODO: Support all integers
            if (skip_keyword_if_is(pos, end, "I32"))
            {
                type_ref.val.get() = ast_integral_keyword{true, 32};
            }

            else if (get_identifier(pos, end) != "")
            {
                ast_symbol_ref sym;
                collect_lookup_symbol(pos, end, sym);
                type_ref.val.get() = sym;
            }

            else
            {
                throw std::runtime_error("Expected type");
            }
        }

        template < typename It >
        void collect_lookup_symbol(It& pos, It end, ast_symbol_ref& typesymbol)
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
        void collect_function_args(It& pos, It end, ast_function& f)
        {
            skip_wsc(pos, end);
            // expect_more(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                return;
            }

            ast_function_arg arg;

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
            ast_type_ref& arg_type = arg.type;
            arg_type.val.get() = ast_symbol_ref{};

            // TODO: Collect type ref
            ast_symbol_ref& arg_type_symbol = std::get< ast_symbol_ref >(arg_type.val.get());

            arg_name = get_skip_identifier(pos, end);
            if (arg_name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            skip_wsc(pos, end);

            // TODO: collect_type_symbol
            arg_type.val.get() = ast_symbol_ref{};
            collect_lookup_symbol(pos, end, std::get< ast_symbol_ref >(arg_type.val.get()));

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
        void collect_function_body(It& pos, It end, ast_function& f)
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

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
