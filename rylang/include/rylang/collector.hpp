//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
#define RPNX_RYANSCRIPT1031_COLLECTOR_HEADER

#include <iostream>
#include <string>
#include <vector>

#include "rylang/ast/entity_ast.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/ast/function_arg_ast.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/ast/member_variable_ast.hpp"
#include "rylang/ast/symbol_ref_ast.hpp"
#include "rylang/converters/qual_converters.hpp"
#include "rylang/cow.hpp"
#include "rylang/data/expression_multiply.hpp"
#include "rylang/data/expression_numeric_literal.hpp"
#include "rylang/data/proximate_lookup_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/ex/unexpected_eof.hpp"
#include "rylang/parser.hpp"

namespace rylang
{
    class collector
    {
        friend class collector_tester;
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
        auto parse_number(It begin, It end) -> It
        {
            auto pos = begin;
            if (pos != end && std::isdigit(*pos))
            {
                pos++;
            }
            else
            {
                return pos;
            }

            bool havedot = false;

            while (pos != end && std::isdigit(*pos) || (*pos == '.' && !havedot))
            {
                if (*pos == '.')
                {
                    havedot = true;
                }
                pos++;
            }

            return pos;
        }

        template < typename It >
        std::string get_skip_number(It& pos, It end)
        {
            auto start = pos;
            pos = parse_number(pos, end);
            return std::string(start, pos);
        }

        template < typename It >
        void expect_more(It& pos, It end)
        {
            if (pos == end)
            {
                throw std::runtime_error("Unexpected end of input text.");
            }
        }

        entity_ast* enter_entity2(std::string const& name, bool is_field)
        {
            assert(m_symbol_stack.size() == m_entity_stack.size());

            if (m_entity_stack.size() == 0)
            {
                m_entity_stack.push_back(&m_root);
                m_symbol_stack.push_back("MAIN");
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
                    // current_entity->m_name += str;
                    if (zi != m_symbol_stack.size() - 1)
                    {
                        //    current_entity->m_name += "::";
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
        auto current_entity2() -> Ast*
        {
            auto ent = current_entity();

            if (std::holds_alternative< null_object_ast >(ent->m_specialization.get()))
            {
                ent->m_specialization = Ast{};
            }
            else if (!std::holds_alternative< Ast >(ent->m_specialization.get()))
            {
                throw std::runtime_error("Entity already exists with different category");
            }
            auto& v = ent->m_specialization.get();

            return &std::get< Ast >(v);
        }

        entity_ast* current_entity()
        {
            assert(m_entity_stack.empty() == false);
            return m_entity_stack.back();
        }

        entity_ast* parent_entity()
        {
            assert(m_entity_stack.size() > 1);
            return m_entity_stack[m_entity_stack.size() - 2];
        }

        std::string const& current_entity_name()
        {
            assert(m_symbol_stack.empty() == false);
            return m_symbol_stack.back();
        }

        std::string const& parent_entity_name()
        {
            assert(m_symbol_stack.size() > 1);
            return m_symbol_stack[m_symbol_stack.size() - 2];
        }

      public:
        std::function< void(std::string const&, function_ast const&) > emit_function;
        // std::function< void(std::string const&, class_ast const&) > emit_class;

        template < typename It >
        void collect_file(It begin, It end, file_ast& output)
        {
            It& pos = begin;
            skip_wsc(pos, end);
            if (!skip_keyword_if_is(pos, end, "MODULE"))
            {
                throw std::runtime_error("expected module here");
            }

            skip_wsc(pos, end);

            std::string module_name = get_skip_identifier(pos, end);

            output.module_name = module_name;
            skip_wsc(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::runtime_error("Expected ; here");
            }

            skip_wsc(pos, end);

            if (skip_keyword_if_is(pos, end, "IMPORT"))
            {
                skip_wsc(pos, end);
                std::string module_name = get_skip_identifier(pos, end);
                skip_wsc(pos, end);

                std::string import_name = module_name;

                if (skip_symbol_if_is(pos, end, "AS"))
                {
                    skip_wsc(pos, end);
                    import_name = get_skip_identifier(pos, end);
                    skip_wsc(pos, end);
                }

                if (!skip_symbol_if_is(pos, end, ";"))
                {
                    throw std::runtime_error("Expected ; here");
                }

                output.imports[import_name] = module_name;
            }

            collect(begin, end);
            output.root = this->m_root;
        }

        template < typename It >
        void collect(It begin, It end)
        {
            auto pos = begin;

            skip_wsc(pos, end);

            if (pos == end)
                return;

            while (try_collect_entity(pos, end))
                ;

            auto distance = std::distance(pos, end);

            auto use_distance = std::min< std::size_t >(64, distance);

            if (pos != end)
            {
                throw std::runtime_error("Unexpected text: " + std::string(pos, pos + use_distance));
            }
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

            auto func_entity = current_entity2< functum_entity_ast >();

            skip_wsc(pos, end);

            function_ast f;
            collect_function(pos, end, f);
            auto ent = current_entity();
            if (ent->m_is_field_entity)
            {
                f.this_type = context_reference{};
            }

            func_entity->m_function_overloads.push_back(std::move(f));
            return true;
        }

        template < typename It >
        std::string remaining(It pos, It end)
        {
            return std::string(pos, end);
        }

        enum class functype { regular, constructor, destructor };

        template < typename It >
        void collect_function_delegates(It& pos, It end, function_ast& f)
        {
            std::string r = remaining(pos, end);
            // std::string remaining = std::string(pos, end);
            skip_wsc(pos, end);
            if (!skip_symbol_if_is(pos, end, ":"))
            {
                // No delegates
                return;
            }

        get_delegate:
            function_delegate fd;

            skip_wsc(pos, end);

            fd.target = collect_qualified_symbol(pos, end);

            skip_wsc(pos, end);
            if (skip_symbol_if_is(pos, end, ":("))
            {
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    goto done_args;
                }
            collect_arg:
                skip_wsc(pos, end);
                expression expr = collect_expression(pos, end);
                fd.args.push_back(std::move(expr));
                skip_wsc(pos, end);
                if (skip_symbol_if_is(pos, end, ","))
                    goto collect_arg;
                else if (skip_symbol_if_is(pos, end, ")"))
                    goto done_args;
                else
                    throw std::runtime_error("Expected ',' or ')'");
            }
            else
            {
                throw std::logic_error("Expected ':(...)'");
            }
        done_args:
            f.delegates.push_back(std::move(fd));

            if (skip_symbol_if_is(pos, end, ","))
            {
                goto get_delegate;
            }

            r = remaining(pos, end);
        }

        template < typename It >
        void try_collect_function_specifiers(It& pos, It it_end, function_ast& f)
        {
        start:

            skip_wsc(pos, it_end);

            if (skip_keyword_if_is(pos, it_end, "PRIORITY"))
            {
                if (f.priority.has_value())
                {
                    throw std::runtime_error("PRIORITY already specified");
                }

                skip_wsc(pos, it_end);

                if (!skip_symbol_if_is(pos, it_end, "("))
                {
                    throw std::runtime_error("Expected '(' after PRIORITY");
                }

                auto num = get_skip_number(pos, it_end);
                if (num.empty())
                {
                    // TODO: support expressions here
                    throw std::runtime_error("Expected number after PRIORITY(");
                }

                skip_wsc(pos, it_end);

                if (!skip_symbol_if_is(pos, it_end, ")"))
                {
                    throw std::runtime_error("Expected ')' after PRIORITY(");
                }

                f.priority = std::stoi(num);

                goto start;
            }
        }

        // Call this function after "FUNCTION",
        template < typename It >
        void collect_function(It& pos, It it_end, function_ast& f, functype type = functype::regular)
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

            if (type == functype::constructor)
            {
                assert(current_entity()->m_is_field_entity);
                collect_function_delegates(pos, it_end, f);
            }

            std::string remaining2(pos, it_end);
            skip_wsc(pos, it_end);
            try_collect_function_specifiers(pos, it_end, f);
            try_collect_function_return_type(pos, it_end, f);
            collect_function_body(pos, it_end, f);

            if (current_entity()->m_is_field_entity && !f.this_type.has_value())
            {
                f.this_type = context_reference{};
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

            auto class_entity = current_entity2< class_entity_ast >();

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

            while (skip_wsc(pos, it_end) || try_collect_class_member(pos, it_end) || try_collect_entity(pos, it_end) || try_collect_class_keyword(pos, it_end))
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

            auto namespace_entity = current_entity2< namespace_entity_ast >();

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
        void do_collect_constructor(It& pos, It it_end)
        {
            auto ent = enter_entity2("CONSTRUCTOR", true);

            auto func_entity = current_entity2< functum_entity_ast >();

            skip_wsc(pos, it_end);

            function_ast f;
            collect_function(pos, it_end, f, functype::constructor);
            func_entity->m_function_overloads.push_back(std::move(f));
            leave_entity(ent);
            return;
        }

        template < typename It >
        bool try_collect_class_keyword(It& pos, It it_end)
        {
            class_entity_ast* class_entity = current_entity2< class_entity_ast >();

            std::set< std::string > keywords = {"NO_MOVE", "NO_COPY", "TRIVIALLY_COPYABLE", "RELOCATABLE", "NO_DEFAULT_CONSTRUCTOR", "NO_RELOCATE"};

            std::string next_kw = get_keyword(pos, it_end);
            if (keywords.contains(next_kw))
            {
                class_entity->m_keywords.insert(next_kw);
                skip_keyword_if_is(pos, it_end, next_kw);
                skip_wsc(pos, it_end);

                if (!skip_symbol_if_is(pos, it_end, ";"))
                {
                    throw std::runtime_error("Expected ';' after " + next_kw);
                }
                return true;
            }

            return false;
        }

        template < typename It >
        bool try_collect_class_member(It& pos, It it_end)
        {
            if (!skip_symbol_if_is(pos, it_end, "."))
            {
                return false;
            }

            if (skip_keyword_if_is(pos, it_end, "CONSTRUCTOR"))
            {
                do_collect_constructor(pos, it_end);
                return true;
            }

            std::string name = get_skip_identifier(pos, it_end);

            if (name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            // TODO: This is messsy

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

            auto variable_entity = current_entity2< variable_entity_ast >();

            skip_wsc(begin, end);

            auto name = current_entity_name();
            // now type

            auto parent = parent_entity();
            if (parent->type() == entity_type::class_type)
            {
                auto& cls = parent->get_as< class_entity_ast >();
                cls.m_var_order.push_back(name);
            }

            // type_ref_ast typ;
            qualified_symbol_reference typ;
            // collect_type_symbol(begin, end, typ);
            typ = collect_qualified_symbol(begin, end);
            skip_wsc(begin, end);
            if (!skip_symbol_if_is(begin, end, ";"))
            {
                std::string remaining(begin, end);
                throw std::runtime_error("Expected ';' after variable type");
            }

            variable_entity->m_variable_type = typ;

            return true;
        }

        template < typename It >
        bool try_get_integral_keyword(It& pos, It end, primitive_type_integer_reference& ast)
        {
            auto it = pos;

            if (it != end && (*it == 'I' || *it == 'U'))
            {
                ast.has_sign = *it++ == 'I';

                auto dig_start = it;
                while (it != end && std::isdigit(*it))
                {
                    ++it;
                }

                std::string dig_str = std::string(dig_start, it);
                ast.bits = (int)std::stoi(dig_str.c_str());

                pos = it;
                return true;
            }

            return false;
        }

        template < typename It >
        void collect_lookup_chain(It& pos, It end, lookup_chain& output)
        {
            skip_wsc(pos, end);

            output.chain = {};

            do
            {
                std::string name = get_skip_identifier(pos, end);
                if (name.empty())
                {
                    throw std::runtime_error("Expected identifier");
                }
                lookup_singular lk;
                lk.type = lookup_type::scope;
                lk.identifier = name;
                output.chain.push_back(lk);
                skip_wsc(pos, end);
            } while (skip_symbol_if_is(pos, end, "::"));
        }

        template < typename It >
        bool try_collect_qualified_symbol(It& pos, It end, std::optional< qualified_symbol_reference >& outputv)
        {

            qualified_symbol_reference output = context_reference{};
            std::string remaining = std::string(pos, end);
            skip_wsc(pos, end);
        start:
            primitive_type_integer_reference intr;
            if (try_get_integral_keyword(pos, end, intr))
            {
                output = intr;
            }
            else if (skip_keyword_if_is(pos, end, "T"))
            {
                template_reference tref;

                skip_wsc(pos, end);
                if (skip_symbol_if_is(pos, end, "("))
                {
                    skip_wsc(pos, end);
                    tref.name = get_skip_identifier(pos, end);
                    if (tref.name.empty())
                    {
                        throw std::runtime_error("Expected identifier after T(");
                    }
                    skip_wsc(pos, end);
                    if (!skip_symbol_if_is(pos, end, ")"))
                    {
                        throw std::runtime_error("Expected ')' after T(" +tref.name);
                    }
                }

                output = tref;
            }
            else if (skip_keyword_if_is(pos, end, "MUT"))
            {
                if (!skip_symbol_if_is(pos, end, "&"))
                {
                    // TODO: Support MUT-> etc
                    throw std::runtime_error("Expected & after MUT");
                }
                outputv = mvalue_reference{collect_qualified_symbol(pos, end)};
                return true;
            }
            else if (skip_symbol_if_is(pos, end, "::"))
            {
                // TODO: Support multiple modules
                output = module_reference{"main"};

                auto ident = get_skip_subentity(pos, end);
                if (ident.empty())
                    throw std::runtime_error("expected identifier after ::");

                output = subentity_reference{std::move(output), std::move(ident)};
            }
            else if (skip_symbol_if_is(pos, end, "."))
            {
                std::string remaining = std::string(pos, end);
                auto ident = get_skip_subentity(pos, end);
                if (ident.empty())
                {
                    return false;
                }
                output = subdotentity_reference{std::move(output), std::move(ident)};
            }
            else if (skip_symbol_if_is(pos, end, "->"))
            {
                outputv = instance_pointer_type{collect_qualified_symbol(pos, end)};
                return true;
            }
            else
            {
                std::string remaining = std::string(pos, end);
                auto ident = get_skip_subentity(pos, end);
                if (ident.empty())
                {
                    return false;
                }
                output = subentity_reference{std::move(output), std::move(ident)};
            }

        check_next:
            skip_wsc(pos, end);

            if (remaining.starts_with("I32::") || remaining.starts_with("::CON") || remaining.starts_with("CON"))
            {
                int x = 0;
            }

            remaining = std::string(pos, end);

            if (skip_symbol_if_is(pos, end, "::"))
            {
                auto ident = get_skip_subentity(pos, end);
                if (ident.empty())
                {
                    outputv = output;
                    return true;
                }

                output = subentity_reference{std::move(output), std::move(ident)};
                goto check_next;
            }
            else if (skip_symbol_if_is(pos, end, "::."))
            {
                auto ident = get_skip_subentity(pos, end);
                if (ident.empty())
                {
                    outputv = output;
                    return true;
                }

                output = subdotentity_reference{std::move(output), std::move(ident)};
                goto check_next;
            }
            else if (skip_symbol_if_is(pos, end, "@("))
            {
                instanciation_reference param_set;
                param_set.callee = std::move(output);

                skip_wsc(pos, end);
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    output = param_set;
                    goto check_next;
                }
            next_arg:
                skip_wsc(pos, end);
                param_set.parameters.push_back(collect_qualified_symbol(pos, end));
                skip_wsc(pos, end);
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    output = param_set;
                    goto check_next;
                }
                else if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw std::runtime_error("expected ',' or ')'");
                }
                goto next_arg;
            }

            outputv = output;
            return true;
        }

        template < typename It >
        qualified_symbol_reference collect_qualified_symbol(It& pos, It end)
        {
            std::optional< qualified_symbol_reference > output;
            try_collect_qualified_symbol(pos, end, output);
            if (!output.has_value())
                throw std::runtime_error("Expected qualified symbol");
            return output.value();
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
            qualified_symbol_reference& arg_type = arg.type;

            arg_name = get_skip_identifier(pos, end);
            if (arg_name.empty())
            {
                throw std::runtime_error("Expected identifier");
            }

            skip_wsc(pos, end);

            arg_type = collect_qualified_symbol(pos, end);

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
        void try_collect_function_return_type(It& pos, It end, function_ast& f)
        {
            skip_wsc(pos, end);

            std::string remaining = std::string(pos, end);
            if (skip_symbol_if_is(pos, end, ":"))
            {
                skip_wsc(pos, end);

                f.return_type = collect_qualified_symbol(pos, end);

                skip_wsc(pos, end);
            }
        }

        template < typename It >
        void collect_function_body(It& pos, It end, function_ast& f)
        {
            skip_wsc(pos, end);

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

            std::optional< function_statement > statement;

            while (try_collect_statement(pos, end, statement))
            {
                f.body.statements.push_back(std::move(statement.value()));
                skip_wsc(pos, end);

                if (skip_symbol_if_is(pos, end, "}"))
                {
                    // end of function body
                    return;
                }
            }

            skip_wsc(pos, end);

            if (skip_symbol_if_is(pos, end, "}"))
            {
                // end of function body
                return;
            }
            auto remaining = std::string(pos, end);
            throw std::runtime_error("Expected '}' or statement");
        }

        template < typename It >
        bool try_collect_function_callsite_expression(It& pos, It end, std::optional< expression_call >& output)
        {
            output.reset();

            skip_wsc(pos, end);

            if (!skip_symbol_if_is(pos, end, "("))
                return false;

            expression_call result;

            skip_wsc(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                output = std::move(result);
                return true;
            }
        get_arg:

            expression expr = collect_expression(pos, end);
            result.args.push_back(std::move(expr));

            if (skip_symbol_if_is(pos, end, ","))
            {
                goto get_arg;
            }

            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::runtime_error("expected ',' or ')'");
            }

            output = std::move(result);
            return true;
        }

        template < typename Operator, typename It >
        bool implement_binary_operator_v2(It& pos, It end, std::vector< expression* >& operator_bindings, expression*& value_binding)
        {
            static const constexpr int priority = Operator::priority;
            static constexpr const char* const operator_string = Operator::symbol;
            if (skip_symbol_if_is(pos, end, operator_string))
            {
                skip_wsc(pos, end);
                Operator new_expression;

                expression* binding_point2 = operator_bindings[priority];
                new_expression.lhs = std::move(*binding_point2);
                *binding_point2 = rylang::expression(new_expression);
                expression* binding_pointer = &boost::get< Operator >(*binding_point2).rhs;
                for (int i = priority + 1; i < operator_bindings.size(); i++)
                {
                    operator_bindings[i] = binding_pointer;
                }
                value_binding = &(boost::get< Operator >(*binding_point2)).rhs;
                return true;
            }
            else
                return false;
        }

        template < typename It >
        bool binary_operator_v3(It& pos, It end, std::vector< expression* >& operator_bindings, expression*& value_binding)
        {
            static std::map< std::string, int > operators_map = {
                // clang-format off

                // Assignment operators
                {":=", 0}, {":<", 0},

                // Logical operators
                {"&&", 1}, // and
                {"!&", 1}, // nand
                {"^^", 1}, // xor
                {"!|", 1}, // nor
                {"||", 1}, // or
                {"^>", 1}, // implies
                {"^<", 1}, // implied
                {"!^", 1}, // equilvalent/nxor

                // Comparison operators
                {"==", 2}, {"!=", 2}, {"<=", 2}, {">=", 2}, {"<", 2}, {">", 2},
                
                // Division and modulus
                {"/", 3}, {"%", 3},
                
                // Addition and subtraction
                {"+", 4}, {"-", 4}, // regular
                // TODO:
                //{"+~", 4}, {"-~", 4}, // wrap-around
                //{"+!", 4}, {"-!", 4}, // undefined overflow

                // Multiplication
                {"*", 5},

                // Exponenciation
                {"^", 6},

                // Bitwise operators, same as logical except begin with a dot.
                {".&&", 7}, // bitwise and
                {".!&", 7}, // bitwise nand
                {".^^", 7}, // bitwise xor
                {".!|", 7}, // bitwise nor
                {".||", 7}, // bitwise or
                {".^>", 7}, // bitwise implies
                {".^<", 7}, // bitwise implied
                {".!^", 7}, // bitwise equilvalent

                // plus some additional shift and rotation operators
                {".<<", 7}, {".+>>", 7}, {".>>", 7}, {".@<", 7}, {".@>", 7}
                // clang-format on
            };

            std::string sym = get_symbol(pos, end);
            auto it = operators_map.find(sym);
            if (it == operators_map.end())
                return false;

            skip_symbol_if_is(pos, end, sym);

            skip_wsc(pos, end);

            int priority = it->second;

            expression_binary new_expression;
            new_expression.operator_str = sym;

            expression* binding_point2 = operator_bindings[priority];
            new_expression.lhs = std::move(*binding_point2);
            *binding_point2 = rylang::expression(new_expression);
            expression* binding_pointer = &boost::get< expression_binary >(*binding_point2).rhs;
            for (int i = priority + 1; i < operator_bindings.size(); i++)
            {
                operator_bindings[i] = binding_pointer;
            }
            value_binding = &(boost::get< expression_binary >(*binding_point2)).rhs;
            return true;
        }

        template < typename Operator, typename It >
        bool implement_binary_operator(It& pos, It end, std::string operator_string, int priority, std::vector< expression* >& operator_bindings, expression*& value_binding)
        {
            if (skip_symbol_if_is(pos, end, operator_string))
            {
                skip_wsc(pos, end);
                Operator new_expression;

                expression* binding_point2 = operator_bindings[priority];
                new_expression.lhs = std::move(*binding_point2);
                *binding_point2 = rylang::expression(new_expression);
                expression* binding_pointer = &boost::get< Operator >(*binding_point2).rhs;

                for (int i = priority; i < operator_bindings.size(); i++)
                {
                    operator_bindings[i] = binding_pointer;
                }
                value_binding = &(boost::get< Operator >(*binding_point2)).rhs;
                return true;
            }
            else
                return false;
        }

        template < typename It >
        expression collect_expression(It& pos, It end)
        {
            std::optional< expression > output;
            if (!try_collect_expression(pos, end, output))
            {
                throw std::runtime_error("Expected expression");
            }
            return output.value();
        }

        template < typename It >
        bool try_collect_numeric_literal(It& pos, It end, std::optional< expression_numeric_literal >& num)
        {
            expression_numeric_literal numv;

            std::string number = get_skip_number(pos, end);
            if (number.empty())
                return false;

            numv.value = number;
            num = numv;
            // TODO: integer suffixes.
            return true;
        }

        template < typename It >
        bool try_collect_expression(It& pos, It end, std::optional< expression >& output)
        {
            skip_wsc(pos, end);

            std::string remaining{pos, end};

            expression result;
            std::vector< expression* > bindings;

            bindings.resize(9);

            for (auto& binding : bindings)
            {
                binding = &result;
            }

            expression* value_bind_point = &result;
            bool have_anything = false;

        next_value:

            remaining = std::string(pos, end);
            std::optional< qualified_symbol_reference > sym;
            skip_wsc(pos, end);
            if (auto num_str = get_skip_number(pos, end); !num_str.empty())
            {
                numeric_literal num;
                num.value = num_str;
                *value_bind_point = num;
                have_anything = true;
                skip_wsc(pos, end);
            }
            else if (skip_symbol_if_is(pos, end, "."))
            {
                skip_wsc(pos, end);
                expression_thisdot_reference thisdot;
                thisdot.field_name = get_skip_identifier(pos, end);
                *value_bind_point = thisdot;
                have_anything = true;
            }
            else if (try_collect_qualified_symbol(pos, end, sym))
            {
                assert(sym.has_value());

                expression_symbol_reference lvalue;
                lvalue.symbol = sym.value();

                *value_bind_point = lvalue;
                have_anything = true;
            }
            else if (skip_symbol_if_is(pos, end, "("))
            {

                expression parenthesis;
                parenthesis = collect_expression(pos, end);
                *value_bind_point = parenthesis;
                have_anything = true;
                skip_wsc(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    throw std::runtime_error("Expected ')'");
                }
            }
            else
            {
                if (!have_anything)
                {
                    return false;
                }
                else
                {
                    throw std::runtime_error("Expected binary operator to be followed by value");
                }
            }

        next_operator:
            remaining = std::string(pos, end);

            skip_wsc(pos, end);

#define RYLANG_OPERATOR(X) implement_binary_operator_v2< X >(pos, end, bindings, value_bind_point)

            if (

                // Assignment operators

                binary_operator_v3(pos, end, bindings, value_bind_point))
            {
                goto next_value;
            }
            else if (std::optional< expression_call > call; try_collect_function_callsite_expression(pos, end, call))
            {
                expression* binding_point2 = bindings[bindings.size() - 1];
                call.value().callee = std::move(*binding_point2);
                *binding_point2 = rylang::expression(call.value());
                goto next_operator;
            }
            else if (skip_symbol_if_is(pos, end, "."))
            {
                expression_dotreference dot;
                dot.field_name = get_skip_identifier(pos, end);
                dot.lhs = std::move(*bindings[bindings.size() - 1]);
                *bindings[bindings.size() - 1] = dot;
                goto next_operator;
            }
            else
            {
                output = result;
                return true;
            }
        }

        template < typename It >
        function_if_statement collect_if_statement(It& pos, It end)
        {
            skip_wsc(pos, end);

            if (!skip_keyword_if_is(pos, end, "IF"))
            {
                throw std::runtime_error("Expected 'IF'");
            }

            skip_wsc(pos, end);

            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::runtime_error("Expected '('");
            }

            function_if_statement if_statement;

            if_statement.condition = collect_expression(pos, end);

            skip_wsc(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::runtime_error("Expected ')'");
            }

            skip_wsc(pos, end);

            if_statement.then_block = collect_function_block(pos, end);

            skip_wsc(pos, end);

            if (skip_keyword_if_is(pos, end, "ELSE"))
            {
                skip_wsc(pos, end);
                if_statement.else_block = collect_function_block(pos, end);
            }

            return if_statement;
        }

        template < typename It >
        function_block collect_function_block(It& pos, It end)
        {
            if (!skip_symbol_if_is(pos, end, "{"))
            {
                throw std::runtime_error("Expected a '{' to collect a function block here.");
            }

            function_block block;

            std::optional< function_statement > statement;

            skip_wsc(pos, end);
            while (try_collect_statement(pos, end, statement))
            {
                block.statements.push_back(std::move(statement.value()));
                skip_wsc(pos, end);
                statement.reset();
            }

            if (!skip_symbol_if_is(pos, end, "}"))
            {
                throw std::runtime_error("Expected a '}' to end a function block here.");
            }

            return block;
        }

        template < typename It >
        bool try_collect_expression_statement(It& pos, It end, std::optional< function_expression_statement >& output)
        {
            skip_wsc(pos, end);
            std::string remaining = std::string(pos, end);
            std::optional< expression > expr;
            if (try_collect_expression(pos, end, expr))
            {
                skip_wsc(pos, end);
                remaining = std::string(pos, end);
                if (!skip_symbol_if_is(pos, end, ";"))
                {
                    throw std::runtime_error("Expected ';' after expression");
                }
                function_expression_statement result;
                result.expr = std::move(expr.value());
                output = result;
                return true;
            }
            return false;
        }

        template < typename It >
        function_var_statement collect_var_statement(It& pos, It end)
        {
            skip_wsc(pos, end);

            if (!skip_keyword_if_is(pos, end, "VAR"))
            {
                throw std::runtime_error("Expected 'VAR'");
            }

            skip_wsc(pos, end);

            function_var_statement var_statement;

            var_statement.name = get_skip_identifier(pos, end);

            skip_wsc(pos, end);

            std::string remaining{pos, end};
            var_statement.type = collect_qualified_symbol(pos, end);

            skip_wsc(pos, end);

            if (skip_symbol_if_is(pos, end, ":("))
            {

                while (true)
                {

                    skip_wsc(pos, end);
                    if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }

                    remaining = std::string(pos, end);

                    expression expr = collect_expression(pos, end);
                    var_statement.initializers.push_back(std::move(expr));

                    if (skip_symbol_if_is(pos, end, ","))
                    {
                        continue;
                    }
                    else if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }
                    else
                    {
                        throw std::runtime_error("Expected ',' or ')'");
                    }
                }
            }

            std::string remaining2{pos, end};

            skip_wsc(pos, end);

            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::runtime_error("Expected ';'");
            }

            return var_statement;
        }

        template < typename It >
        function_return_statement collect_return_statement(It& pos, It end)
        {
            if (!skip_keyword_if_is(pos, end, "RETURN"))
            {
                throw std::runtime_error("Expected 'RETURN'");
            }

            function_return_statement output;

            skip_wsc(pos, end);

            if (skip_symbol_if_is(pos, end, ";"))
            {
                return output;
            }

            output.expr = collect_expression(pos, end);

            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::runtime_error("Expected ';'");
            }

            return output;
        }

        template < typename It >
        function_while_statement collect_while_statement(It& pos, It end)
        {
            skip_wsc(pos, end);

            if (!skip_keyword_if_is(pos, end, "WHILE"))
            {
                throw std::runtime_error("Expected 'WHILE'");
            }

            function_while_statement output;

            skip_wsc(pos, end);

            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::runtime_error("Expected '('");
            }

            output.condition = collect_expression(pos, end);

            skip_wsc(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::runtime_error("Expected ')'");
            }
            skip_wsc(pos, end);
            output.loop_block = collect_function_block(pos, end);

            return output;
        }

        template < typename It >
        bool try_collect_statement(It& pos, It end, std::optional< function_statement >& output)
        {
            skip_wsc(pos, end);

            std::string remaining = std::string(pos, end);

            if (remaining.starts_with("I32::CONSTRU"))
            {
                int x = 0;
            }

            std::optional< function_expression_statement > exp_st;

            if (get_keyword(pos, end) == "IF")
            {
                output = collect_if_statement(pos, end);
                return true;
            }
            else if (get_keyword(pos, end) == "VAR")
            {
                output = collect_var_statement(pos, end);
                return true;
            }
            else if (get_keyword(pos, end) == "RETURN")
            {
                output = collect_return_statement(pos, end);
                return true;
            }
            else if (get_keyword(pos, end) == "WHILE")
            {
                output = collect_while_statement(pos, end);
                return true;
            }
            else if (try_collect_expression_statement(pos, end, exp_st))
            {
                output = exp_st;
                return true;
            }

            return false;
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COLLECTOR_HEADER
