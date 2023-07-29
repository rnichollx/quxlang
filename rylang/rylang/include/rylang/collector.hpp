//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
#define RPNX_RYANSCRIPT1031_COLLECTOR_HEADER

#include <iostream>
#include <string>
#include <vector>

#include "rylang/ast/class_ast.hpp"
#include "rylang/ast/entity_ast.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/ast/function_arg_ast.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/ast/member_variable_ast.hpp"
#include "rylang/ast/namespace_ast.hpp"
#include "rylang/ast/symbol_ref_ast.hpp"
#include "rylang/ast/type_ref_ast.hpp"
#include "rylang/ex/unexpected_eof.hpp"
#include "rylang/parser.hpp"

namespace rylang
{
    class collector
    {
        struct symbol_frame
        {
        };
        std::vector< std::string > m_symbol_stack;

        entity_ast m_root;

        std::vector< entity_ast* > m_entity_stack;

      private:
        void push_symbol(std::string symbol)
        {
            m_symbol_stack.push_back(std::move(symbol));
        }
        void pop_symbol()
        {
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

        entity_ast* enter_entity2(std::string const & name, bool is_field)
        {
           assert(m_symbol_stack.size() == m_entity_stack.size());

            if (m_entity_stack.size() == 0)
            {
                m_entity_stack.push_back(&m_root);
                m_symbol_stack.push_back("global");
            }

            push_symbol(name);

            auto& top = *m_entity_stack.back();

            entity_ast* current_entity{};

            auto lookup = top.m_sub_entities.find(name);
            if (lookup == top.m_sub_entities.end())
            {
                auto both = top.m_sub_entities.emplace(name, entity_ast{});
                lookup = both.first;
                current_entity = &lookup->second.get();
                assert(current_entity);
                current_entity->m_is_field_entity = is_field;
                for (std::size_t zi = 0; zi != m_symbol_stack.size(); zi++)
                {
                    if (zi > 5)
                        throw std::runtime_error("Too many namespaces");
                    auto str = m_symbol_stack.at(zi);
                    current_entity->m_name += str;
                    if (zi != m_symbol_stack.size() - 1)
                    {
                        current_entity->m_name += "::";
                    }
                }
            }
            else
            {
                current_entity = &lookup->second.get();
                if (current_entity->m_is_field_entity != is_field)
                {
                    throw std::runtime_error("Entity already exists with different category");
                }
            }

            m_entity_stack.push_back(current_entity);

            assert(m_symbol_stack.size() == m_entity_stack.size());
            return current_entity;
        }


        void leave_entity(entity_ast* which)
        {
            assert(m_entity_stack.size() > 0);
            assert(m_entity_stack.back() == which);
            m_entity_stack.pop_back();
            m_symbol_stack.pop_back();
            assert(m_entity_stack.size() == m_symbol_stack.size());
        }

        template < typename Ast >
        auto current_entity2() -> Ast *
        {
            auto ent = current_entity();

            if (std::holds_alternative<null_object_ast>(ent->m_subvalue.get()))
            {
                ent->m_subvalue = Ast{};
            }
            else if (! std::holds_alternative<Ast>(ent->m_subvalue.get()))
            {
                throw std::runtime_error("Entity already exists with different category");
            }
            auto& v =  ent->m_subvalue.get();

            return &std::get<Ast>(v);
        }

        entity_ast* current_entity()
        {
            assert(m_entity_stack.empty() == false);
            return m_entity_stack.back();
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

            while (try_collect_entity(pos, end))
                ;
        }

        template < typename It >
        bool try_collect_entity(It& pos, It end)
        {
            skip_wsc(pos, end);

            if (!skip_symbol_if_is(pos, end, "::"))
            {
                return false;
            }
            expect_more(pos, end);

            std::string remaining = std::string(pos, end);
            std::string name = get_skip_identifier(pos, end);

            if (name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            auto ent = enter_entity2(name, false);

            skip_wsc(pos, end);

            if (try_collect_function(pos, end) || try_collect_class(pos, end) || try_collect_namespace(pos, end))
            {
                leave_entity(ent);
                return true;
            }

            throw std::runtime_error("Expected FUNCTION, CLASS, or NAMESPACE");
        }

        entity_ast get() const
        {
            return m_root;
        }

        template < typename It >
        bool try_collect_function(It& pos, It end)
        {
            if (!skip_keyword_if_is(pos, end, "FUNCTION"))
            {
                return false;
            }

            auto func_entity = current_entity2<function_entity_ast>();

            skip_wsc(pos, end);

            function_ast f;
            collect_function(pos, end, f);
            func_entity->m_function_overloads.push_back(std::move(f));
            return true;
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
        bool try_collect_class(It& pos, It it_end)
        {
            skip_wsc(pos, it_end);

            if (!skip_keyword_if_is(pos, it_end, "CLASS"))
            {
                return false;
            }

            auto class_entity = current_entity2<class_entity_ast>();

            if (pos == it_end)
            {
                throw unexpected_eof< It >{pos};
            }

            skip_wsc(pos, it_end);

            std::string remaining = std::string(pos, it_end);
            if (!skip_symbol_if_is(pos, it_end, "{"))
            {
                throw std::runtime_error("Expected '{'");
            }

            skip_wsc(pos, it_end);

            while (skip_wsc(pos, it_end) || try_collect_class_member(pos, it_end) || try_collect_entity(pos, it_end))
                ;

            if (!skip_symbol_if_is(pos, it_end, "}"))
            {
                throw std::runtime_error("Expected '}' after CLASS body");
            }

            return true;
        }

        template < typename It >
        bool try_collect_namespace(It& pos, It it_end)
        {
            if (!skip_keyword_if_is(pos, it_end, "NAMESPACE"))
            {
                return false;
            }

            auto namespace_entity = current_entity2<namespace_entity_ast>();

            skip_wsc(pos, it_end);

            if (!skip_symbol_if_is(pos, it_end, "{"))
            {
                throw std::runtime_error("Expected '{' after 'NAMESPACE'");
            }

            while (skip_wsc(pos, it_end) || try_collect_entity(pos, it_end))
                ;

            if (!skip_symbol_if_is(pos, it_end, "}"))
            {
                throw std::runtime_error("Expected '}' after NAMESPACE body");
            }

            return true;
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

                c.member_variables.push_back(member);
            }

            else
            {
                std::string remaining = std::string(pos, it_end);
                throw std::runtime_error("Expected VAR");
            }
        }

        template < typename It >
        bool try_collect_class_member(It& pos, It it_end)
        {
            if (!skip_symbol_if_is(pos, it_end, "."))
            {
                return false;
            }

            std::string name = get_skip_identifier(pos, it_end);

            if (name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            auto ent = enter_entity2(name, true);

            while (skip_wsc(pos, it_end) || try_collect_variable(pos, it_end) || try_collect_function(pos, it_end))
                ;

            leave_entity(ent);

            return true;
        }

        template < typename It >
        bool try_collect_variable(It& begin, It end)
        {
            skip_wsc(begin, end);

            if (!skip_keyword_if_is(begin, end, "VAR"))
            {
                return false;
            }

            auto variable_entity = current_entity2<variable_entity_ast>();

            skip_wsc(begin, end);
            // now type

            type_ref_ast typ;

            collect_type_symbol(begin, end, typ);
            skip_wsc(begin, end);
            if (!skip_symbol_if_is(begin, end, ";"))
            {
                throw std::runtime_error("Expected ';' after variable type");
            }

            variable_entity->m_variable_type = typ;

            return true;
        }

        template < typename It >
        bool try_get_integral_keyword(It& pos, It end, integral_keyword_ast& ast)
        {
            auto it = pos;

            if (it != end && (*it == 'I' || *it == 'U'))
            {
                ast.is_sized = *it++ == 'I';

                auto dig_start = it;
                while (it != end && std::isdigit(*it))
                {
                    ++it;
                }

                std::string dig_str = std::string(dig_start, it);
                ast.size = (int)std::stoi(dig_str.c_str());

                pos = it;
                return true;
            }

            return false;
        }

        template < typename It >
        void collect_type_symbol(It& pos, It end, type_ref_ast& type_ref)
        {
            skip_wsc(pos, end);

            std::string remaining = std::string(pos, end);

            // TODO: Support all integers
            integral_keyword_ast integral;

            if (try_get_integral_keyword(pos, end, integral))
            {
                type_ref.val.get() = integral;
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

            std::string remaining = std::string(pos, end);

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
            collect_type_symbol(pos, end, arg_type);

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
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
