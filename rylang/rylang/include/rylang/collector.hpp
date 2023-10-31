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
#include "rylang/cow.hpp"
#include "rylang/data/expression_multiply.hpp"
#include "rylang/data/proximate_lookup_reference.hpp"
#include "rylang/data/s_expression.hpp"
#include "rylang/data/s_expression_add.hpp"
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

      public:
        std::function< void(std::string const&, function_ast const&) > emit_function;
        std::function< void(std::string const&, class_ast const&) > emit_class;

        template < typename It >
        void collect_file(It begin, It end, file_ast& output)
        {
            skip_wsc(begin, end);
            if (!skip_keyword_if_is(begin, end, "MODULE"))
            {
                throw std::runtime_error("expected module here");
            }

            skip_wsc(begin, end);

            std::string module_name = get_skip_identifier(begin, end);

            output.module_name = module_name;
            skip_wsc(begin, end);
            if (!skip_symbol_if_is(begin, end, ";"))
            {
                throw std::runtime_error("Expected ; here");
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

            auto variable_entity = current_entity2< variable_entity_ast >();

            skip_wsc(begin, end);
            // now type

            // type_ref_ast typ;
            type_reference typ;
            // collect_type_symbol(begin, end, typ);
            collect_type_reference(begin, end, typ);
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
        void collect_type_reference(It& pos, It end, type_reference& type_ref)
        {
            skip_wsc(pos, end);

            std::string remaining = std::string(pos, end);

            if (skip_symbol_if_is(pos, end, "->"))
            {
                pointer_reference ptr;

                collect_type_reference(pos, end, ptr.to);

                type_ref = ptr;

                return;
            }

            else if (skip_symbol_if_is(pos, end, "::"))
            {
                absolute_lookup_reference abs_ref;

                collect_lookup_chain(pos, end, abs_ref.chain);

                type_ref = abs_ref;
                return;
            }
            else if (skip_symbol_if_is(pos, end, ":"))
            {
                proximate_lookup_reference ref;

                collect_lookup_chain(pos, end, ref.chain);

                type_ref = ref;
                return;
            }
            else
            {
                integral_keyword_ast integral;
                if (try_get_integral_keyword(pos, end, integral))
                {
                    type_ref = integral;
                }
                else
                {
                    throw std::runtime_error("expected integral keyword, pointer, or symbol chain");
                }
            }
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
            remaining = std::string(pos, end);
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
        bool try_collect_expression(It& pos, It end, std::optional< expression >& output)
        {
            skip_wsc(pos, end);

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

            std::string remaining = std::string(pos, end);
            skip_wsc(pos, end);
            if (skip_symbol_if_is(pos, end, "."))
            {
                skip_wsc(pos, end);
                expression_thisdot_reference thisdot;
                thisdot.field_name = get_skip_identifier(pos, end);
                *value_bind_point = thisdot;
                have_anything = true;
            }
            else if (get_identifier(pos, end).empty() == false)
            {
                std::string identifier = get_skip_identifier(pos, end);
                // TODO: lookup chain?

                expression_lvalue_reference lvalue;
                lvalue.identifier = identifier;
                *value_bind_point = lvalue;
                have_anything = true;
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
                RYLANG_OPERATOR(expression_move_assign) || RYLANG_OPERATOR(expression_copy_assign) ||
                // Arithmetic operators
                RYLANG_OPERATOR(expression_add) || RYLANG_OPERATOR(expression_subtract) || RYLANG_OPERATOR(expression_multiply) || RYLANG_OPERATOR(expression_divide) ||
                // Logical operators
                RYLANG_OPERATOR(expression_or) || RYLANG_OPERATOR(expression_and) || RYLANG_OPERATOR(expression_nand) || RYLANG_OPERATOR(expression_nor) || RYLANG_OPERATOR(expression_xor) ||
                RYLANG_OPERATOR(expression_implies) || RYLANG_OPERATOR(expression_implied)
                // Comparison operators

            )
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

            if (!skip_symbol_if_is(pos, end, ":"))
            {
                throw std::runtime_error("Expected ':'");
            }

            skip_wsc(pos, end);

            collect_type_reference(pos, end, var_statement.type);

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
        bool try_collect_statement(It& pos, It end, std::optional< function_statement >& output)
        {
            skip_wsc(pos, end);

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
