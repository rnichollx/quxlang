//
// Created by Ryan Nicholl on 11/23/23.
//

#ifndef TO_PRETTY_STRING_HEADER_GUARD
#define TO_PRETTY_STRING_HEADER_GUARD

#include "data/expression.hpp"
#include "data/function_block.hpp"
#include "data/qualified_symbol_reference.hpp"
#include "data/vm_executable_unit.hpp"
#include "data/vm_expression.hpp"
#include <string>

namespace quxlang
{
    // the same idea as to_string, except this one provides an indented version with newlines
    // Rules:
    // 1. Everything returns with a newline at the end
    // 2. Nothing provides initial indentation before the first line
    // 3. Temporarily increase indentation by 1 before adding a sub member
    // 4. should return the result string (if doesn't have this, it's a bug)
    class to_pretty_string_visitor : public boost::static_visitor< std::string >
    {
      private:
        std::size_t current_indent = 0;
        std::size_t indent_step = 2;
        std::string indent_string_m;

        std::string const& indent_string()
        {
            std::size_t ideal_size = current_indent * indent_step;
            while (indent_string_m.size() < ideal_size)
            {
                indent_string_m += ' ';
            }
            while (indent_string_m.size() > ideal_size)
            {
                indent_string_m.pop_back();
            }
            return indent_string_m;
        }

      public:
        std::string operator()(expression_call const& call)
        {
            std::string result;
            result += "expression_call{\n";

            current_indent++;
            result += indent_string() + "interface: <omitted>";
            result += "\n";
            result += indent_string() + "arguments: {\n";
            current_indent++;
            for (auto const& arg : call.args)
            {
                result += indent_string() + "arg: ";
                result += rpnx::apply_visitor<std::string>(*this, arg);
                result += "\n";
            }
            current_indent--;
            result += indent_string() + "} // args\n";
            current_indent--;
            result += indent_string() + "} // expression_call\n";
            return result;
        }

        std::string operator()(vm_expr_primitive_binary_op const& op)
        {
            std::string result;
            result += "vm_expr_primitive_binary_op{ op: ";
            result += op.oper;
            result += "\n";
            ++current_indent;
            result += indent_string();
            result += "left: ";
            result += rpnx::apply_visitor<std::string>(*this, op.lhs);
            result += indent_string();
            result += "right: ";
            result += rpnx::apply_visitor<std::string>(*this, op.rhs);
            result += indent_string() + "} // binary op\n";
            --current_indent;
            return result;
        }

        std::string operator()(void_value const&)
        {
            return "void_value{}\n";
        }

        std::string operator()(vm_block const& block)
        {
            std::string result;
            result += "vm_block{ \n";
            current_indent++;
            result += indent_string() + "comments:\n";
            current_indent++;
            for (auto const& comment : block.comments)
            {
                result += indent_string() + comment + "\n";
            }
            current_indent--;

            result += "\n";
            result += indent_string() + "code: {\n";
            current_indent++;
            for (auto const& unit : block.code)
            {
                result += indent_string();
                result += rpnx::apply_visitor<std::string>(*this, unit);
                result += "\n";
            }
            current_indent--;
            result += indent_string() + "} end code\n";
            current_indent--;
            result += indent_string() + "} end block\n";
            return result;
        }

        std::string operator()(void_type const&)
        {
            return "void_type{}";
        }

        std::string operator()(vm_expr_literal const& op)
        {
            std::string result;
            result += "vm_expr_literal{\n";
            current_indent++;
            result += indent_string();
            current_indent--;
            result += op.literal;
            result += "\n" + indent_string() + "} end literal\n";
            return result;
        }

        std::string operator()(vm_expr_poison const& poison)
        {
            std::string result;
            result += "vm_expr_poison{\n";
            current_indent++;
            result += indent_string();
            result += "type: ";
            result += rpnx::apply_visitor<std::string>(*this, poison.type);
            current_indent--;
            result += indent_string();
            result += "} end poison\n";
            return result;
        }

        std::string operator()(vm_store const& store)
        {
            std::string result;
            result += "vm_store{\n";
            current_indent++;
            result += indent_string();
            result += "what: ";
            result += rpnx::apply_visitor<std::string>(*this, store.what);
            result += indent_string();
            result += "where: ";
            result += rpnx::apply_visitor<std::string>(*this, store.where);
            current_indent--;
            result += indent_string();
            result += "} end store\n";
            return result;
        }

        std::string operator()(vm_expr_access_field const& op)
        {
            std::string result;
            result += "vm_expr_access_field{ field: ";
            current_indent++;
            result += op.offset;
            result += "\n";
            result += indent_string();
            result += "base: ";
            result += rpnx::apply_visitor<std::string>(*this, op.base);
            result += "type: ";
            result += rpnx::apply_visitor<std::string>(*this, op.type);
            current_indent--;
            result += indent_string() + "} end access_field\n";
            return result;
        }

        std::string operator()(vm_expr_store const& op)
        {
            std::string result;
            result += "vm_expr_store{\n";
            current_indent++;
            result += indent_string() + "what: ";
            result += rpnx::apply_visitor<std::string>(*this, op.what);
            result += indent_string() + "where: ";
            result += rpnx::apply_visitor<std::string>(*this, op.where);
            current_indent--;
            result += indent_string() + "} end expr_store\n";
            return result;
        };

        std::string operator()(std::monostate const&) const
        {
            return "ERROR(monostate)\n";
        }

        std::string operator()(vm_disable_storage const&)
        {
            return "vm_disable_storage{}\n";
        }

        std::string operator()(context_reference const& ref)
        {
            return "context_reference{}\n";
        }
        std::string operator()(subentity_reference const& ref)
        {
            std::string result;
            result = "subentity_reference{\n";
            current_indent++;
            result += indent_string();
            result += "parent: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.parent);
            result += "\n";
            result += indent_string();
            result += "subentity_name: ";
            result += ref.subentity_name;
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(subdotentity_reference const& ref)
        {
            std::string result;
            result = "subdotentity_reference{\n";
            current_indent++;
            result += indent_string();
            result += "parent: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.parent);
            result += "\n";
            result += indent_string();
            result += "subdotentity_name: ";
            result += ref.subdotentity_name;
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_expr_call const& field)
        {
            std::string result;
            result = "vm_expr_call{\n";
            current_indent++;
            result += indent_string();
            result += "interface: <omitted>";
            // TODO: Add this later
            // result += boost::apply_visitor(*this, field.interface);
            result += "\n";
            result += indent_string();
            result += "args: ";
            current_indent++;
            for (auto const& arg : field.arguments)
            {
                result += indent_string();
                result += "arg:" + rpnx::apply_visitor<std::string>(*this, arg);
                result += "\n";
            }
            current_indent--;
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_expr_load_address const& field)
        {
            std::string result;
            result = "vm_expr_load_address{\n";
            current_indent++;
            result += indent_string();
            result += "type: ";
            result += rpnx::apply_visitor<std::string>(*this, field.type);
            result += indent_string() + "index: ";
            result += std::to_string(field.index) + "\n";
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_expr_dereference const& field)
        {
            std::string result;
            result = "vm_expr_dereference{\n";
            current_indent++;
            result += indent_string();
            result += "base: ";
            result += rpnx::apply_visitor<std::string>(*this, field.expr);
            result += indent_string() + "type: ";
            result += rpnx::apply_visitor<std::string>(*this, field.type);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_expr_primitive_unary_op const& field)
        {
            std::string result;
            result = "vm_expr_primitive_unary_op{\n";
            current_indent++;
            result += indent_string();
            // todo: this
            result += "op: <omitted>";
            result += "\n";
            result += indent_string();
            result += "base: ";
            result += rpnx::apply_visitor<std::string>(*this, field.expr);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_expr_load_literal const& field)
        {
            std::string result;
            result = "vm_expr_load_literal{\n";
            current_indent++;
            result += indent_string();
            result += "literal: ";
            result += field.literal;
            result += "\n";
            result += indent_string();
            result += "type: ";
            result += rpnx::apply_visitor<std::string>(*this, field.type);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_expr_bound_value const& field)
        {
            std::string result;
            result = "vm_expr_bound_value{\n";
            current_indent++;
            result += indent_string();
            result += "value: ";
            result += rpnx::apply_visitor<std::string>(*this, field.value);
            result += indent_string();
            result += "function_ref: ";
            result += rpnx::apply_visitor<std::string>(*this, field.function_ref);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_expr_reinterpret const& field)
        {
            std::string result;
            result = "vm_expr_reinterpret{\n";
            current_indent++;
            result += indent_string();
            result += "expr: ";
            result += rpnx::apply_visitor<std::string>(*this, field.expr);
            result += indent_string();
            result += "type: ";
            result += rpnx::apply_visitor<std::string>(*this, field.type);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_execute_expression const& expr)
        {
            std::string result;
            result = "vm_execute_expression{\n";
            current_indent++;
            result += indent_string();
            result += "expr: ";
            result += rpnx::apply_visitor<std::string>(*this, expr.expr);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_return const&)
        {
            return "vm_return{}\n";
        }

        std::string operator()(vm_if const& ifval)
        {
            std::string result;
            result = "vm_if{\n";
            current_indent++;
            result += indent_string();
            result += "condition: ";
            result += rpnx::apply_visitor<std::string>(*this, ifval.condition);
            result += indent_string();
            result += "then_block: ";
            result += operator()(ifval.then_block);
            if (ifval.else_block)
            {
                result += indent_string();
                result += "else_block: ";
                result += operator()(*ifval.else_block);
            }
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(vm_while const& whileval)
        {
            std::string result;
            result = "vm_while{\n";
            current_indent++;
            result += indent_string();
            result += "condition: ";
            result += rpnx::apply_visitor<std::string>(*this, whileval.condition);
            result += indent_string();
            result += "loop_block: ";
            result += operator()(whileval.loop_block);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(expression_this_reference const&)
        {
            return "expression_this_reference{}\n";
        }

        std::string operator()(expression_thisdot_reference const& what)
        {
            return "expression_thisdot_reference{ field_name: " + what.field_name + " }\n";
        }

        std::string operator()(vm_enable_storage const& field)
        {
            std::string result;
            result = "vm_enable_storage{\n";
            current_indent++;
            result += indent_string();
            result += "index: ";
            result += std::to_string(field.index);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(module_reference const& module)
        {
            return "module{ " + module.module_name + " }\n";
        }

        std::string operator()(primitive_type_integer_reference const& inttype)
        {
            return "inttype{" + std::to_string(inttype.bits) + " }\n";
            // TODO: sign
        }

        std::string operator()(primitive_type_bool_reference const& booltype)
        {
            return "booltype{}\n";
        }

        std::string operator()(instance_pointer_type const& ptr)
        {
            std::string result;
            result = "pointer_to_reference{\n";
            current_indent++;
            result += indent_string();
            result += "target: ";
            result += rpnx::apply_visitor<std::string>(*this, ptr.target);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(value_expression_reference const& val)
        {
            // TODO: impelment value expressions
            return "value_expression_reference{<todo>}\n";
        }

        std::string operator()(instanciation_reference const& func)
        {
            std::string result;
            result = "functanoid_reference{\n";
            current_indent++;
            result += indent_string();
            result += "callee: ";
            result += rpnx::apply_visitor<std::string>(*this, func.callee);
            result += indent_string();
            result += "parameters: ";
            current_indent++;
            for (auto const& param : func.parameters)
            {
                result += indent_string();
                result += "param: ";
                result += rpnx::apply_visitor<std::string>(*this, param);
                result += "\n";
            }
            current_indent--;
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(tvalue_reference const& ref)
        {
            std::string result;
            result = "tvalue_reference{\n";
            current_indent++;
            result += indent_string();
            result += "target: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.target);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(mvalue_reference const& ref)
        {
            std::string result;
            result = "mvalue_reference{\n";
            current_indent++;
            result += indent_string();
            result += "target: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.target);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(cvalue_reference const& ref)
        {
            std::string result;
            result = "cvalue_reference{\n";
            current_indent++;
            result += indent_string();
            result += "target: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.target);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(ovalue_reference const& ref)
        {
            std::string result;
            result = "ovalue_reference{\n";
            current_indent++;
            result += indent_string();
            result += "target: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.target);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(avalue_reference const& ref)
        {
            std::string result;
            result = "avalue_reference{\n";
            current_indent++;
            result += indent_string();
            result += "target: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.target);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(bound_function_type_reference const& ref)
        {
            std::string result;
            result = "bound_function_type_reference{\n";
            current_indent++;
            result += indent_string();
            result += "object_type: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.object_type);
            result += indent_string();
            result += "function_type: ";
            result += rpnx::apply_visitor<std::string>(*this, ref.function_type);
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(numeric_literal_reference const& ref)
        {
            return "numeric_literal_reference{}\n";
        }

        std::string operator()(expression_symbol_reference const& symbol)
        {
            return "expression_symbol_reference{ symbol: " + to_string(symbol.symbol) + " }\n";
        }

        std::string operator()(expression_dotreference const& what)
        {
            std::string result;
            result = "expression_dotreference{\n";
            current_indent++;
            result += indent_string();
            result += "field_name: " + what.field_name + "\n";
            result += indent_string();
            result += "base: ";
            result += rpnx::apply_visitor<std::string>(*this, what.lhs);
            result += indent_string();
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(numeric_literal const& literal)
        {
            return "numeric_literal{ " + literal.value + " }\n";
        }

        std::string operator()(expression_binary const& what)
        {
            std::string result;
            result = "expression_binary{\n";
            current_indent++;
            result += indent_string();
            result += "op: ";
            result += what.operator_str;
            result += "\n";
            result += indent_string();
            result += "lhs: ";
            result += rpnx::apply_visitor<std::string>(*this, what.lhs);
            result += indent_string();
            result += "rhs: ";
            result += rpnx::apply_visitor<std::string>(*this, what.rhs);
            result += indent_string();
            current_indent--;
            result += indent_string() + "}\n";
            return result;
        }

        std::string operator()(template_reference const& ref)
        {
            return "template_reference{ name: " + ref.name + " }\n";
        }
    };

    inline std::string to_pretty_string(expression expr)
    {
        to_pretty_string_visitor visitor;
        return rpnx::apply_visitor<std::string>(visitor, expr);
    }

    inline std::string to_pretty_string(vm_executable_unit expr)
    {
        to_pretty_string_visitor visitor;
        return rpnx::apply_visitor<std::string>(visitor, expr);
    }

} // namespace quxlang

#endif // TO_PRETTY_STRING_HEADER_GUARD
