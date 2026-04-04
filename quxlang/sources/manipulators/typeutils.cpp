// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/manipulators/typeutils.hpp"

#include "quxlang/data/type_symbol.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/manipulators/expression_stringifier.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/value.hpp"
#include "quxlang/data/codegen_types.hpp"

namespace quxlang
{
    struct array_type;

    struct expression_stringifier
    {
        expression_stringifier()
        {
        }

        std::string operator()(expression_bits const& bits) const
        {
            return "BITS(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_typecast const& cast) const
        {
            std::string result = "( " + to_string(cast.expr) + " AS ";

            if (cast.keyword)
            {
                result += cast.keyword.value() + " ";
            }

            result += to_string(cast.to_type) + " )";

            return result;
        }

        std::string operator()(expression_pun const& expr) const
        {
            return "( PUN " + to_string(expr.value) + " AS " + to_string(expr.as_type) + " )";
        }

        std::string operator()(expression_place const& expr) const
        {
            std::string result = "( PLACE AT(" + to_string(expr.at) + ") " + to_string(expr.type);
            if (expr.assign_init)
            {
                result += " := " + to_string(*expr.assign_init);
            }
            else if (!expr.args.empty())
            {
                result += ":(";
                for (std::size_t i = 0; i < expr.args.size(); i++)
                {
                    if (i != 0)
                    {
                        result += ", ";
                    }
                    result += to_string(expr.args[i].value);
                }
                result += ")";
            }
            result += " )";
            return result;
        }

        std::string operator()(expression_sizeof const& bits) const
        {
            return "SIZEOF(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_is_signed const& bits) const
        {
            return "IS_SIGNED(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_is_integral const& bits) const
        {
            return "IS_INTEGRAL(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_same_types const& types) const
        {
            return "SAME_TYPES(" + to_string(types.lhs_type) + ", " + to_string(types.rhs_type) + ")";
        }

        std::string operator()(expression_unary_postfix const& be) const
        {
            return "(" + to_string(be.lhs) + " " + be.operator_str + ")";
        }

        std::string operator()(expression_unary_prefix const& be) const
        {
            return "(" + be.operator_str + " " + to_string(be.rhs) + ")";
        }

        std::string operator()(expression_value_keyword const& kw) const
        {
            return kw.keyword;
        }

        std::string operator()(expression_static_choose const& expr) const
        {
            return "STATIC_CHOOSE( " + to_string(expr.condition) + " , " + to_string(expr.true_expr) + " , " + to_string(expr.false_expr) + " )";
        }

        std::string operator()(expression_choose const& expr) const
        {
            return "CHOOSE( " + to_string(expr.condition) + " , " + to_string(expr.true_expr) + " , " + to_string(expr.false_expr) + " )";
        }

        std::string operator()(expression_multibind const& brkts) const
        {
            std::string result;
            result += to_string(brkts.lhs);
            result += " [ ";
            for (std::size_t i = 0; i < brkts.bracketed.size(); i++)
            {
                result += to_string(brkts.bracketed[i]);
                if (i != brkts.bracketed.size() - 1)
                    result += " , ";
            }
            result += " ]";
            return result;
        }

        std::string operator()(expression_numeric_literal const& expr) const
        {
            return expr.value;
        }

        std::string operator()(expression_dotreference const& expr) const
        {
            return "(" + to_string(expr.lhs) + "." + expr.field_name + ")";
        }

        std::string operator()(expression_call const& expr) const
        {
            std::string result = "(" + to_string(expr.callee) + "(";
            for (int i = 0; i < expr.args.size(); i++)
            {
                auto arg = expr.args[i].value;
                result += to_string(arg);
                if (i != expr.args.size() - 1)
                    result += ", ";
            }
            result += "))";
            return result;
        }


        std::string operator()(expression_multiply const& expr) const
        {
            return "(" + to_string(expr.lhs) + " * " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_binary const& expr) const
        {
            return "(" + to_string(expr.lhs) + " " + expr.operator_str + " " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_this_reference const& expr) const
        {
            return "THIS";
        }

        std::string operator()(expression_thisdot_reference const& expr) const
        {
            return "." + expr.field_name;
        }

        std::string operator()(expression_symbol_reference const& expr) const
        {
            return to_string(expr.symbol);
        }

        std::string operator()(expression_target const& expr) const
        {
            return "TARGET" + expr.target;
        }

        std::string operator()(expression_char_literal const& expr) const
        {
            auto val = static_cast< char >(expr.value);

            if (val == '\n')
                return "'\\n'";
            else if (val == '\t')
                return "'\\t'";
            else if (val == '\r')
                return "'\\r'";
            else if (val == '\'')
                return "'\\''";
            else if (val == '\\')
                return "'\\\\'";
            else if (val == '\0')
                return "'\\0'";
            else
            {
                return std::string("'") + val + "'";
            }
        }

        std::string operator()(expression_string_literal const& expr) const
        {
            // TODO: Escape
            return "\"" + expr.value + "\"";
        }

        std::string operator()(expression_leftarrow const& expr) const
        {
            std::string output = "(" + to_string(expr.lhs) + " <- )";
            return output;
        }

        std::string operator()(expression_rightarrow const& expr) const
        {
            std::string output = "(" + to_string(expr.lhs) + " -> )";
            return output;
        }
    };

    struct type_symbol_stringifier
    {
        std::string operator()(storage const& ref) const;
        std::string operator()(aligned_storage const& ref) const;
        std::string operator()(readonly_constant const& ref) const;
        std::string operator()(freebound_identifier const& ref) const;
        std::string operator()(context_reference const& ref) const;
        std::string operator()(subsymbol const& ref) const;
        std::string operator()(initialization_reference const& ref) const;
        std::string operator()(absolute_module_reference const& ref) const;
        std::string operator()(attached_type_reference const& ref) const;
        std::string operator()(int_type const& ref) const;

        std::string operator()(byte_type const& ref) const;
        std::string operator()(initguard_type const& ref) const;
        std::string operator()(initguard_lock_type const& ref) const;
        std::string operator()(bool_type const& ref) const;
        std::string operator()(array_type const& arr) const;
        std::string operator()(size_type const& ref) const;
        std::string operator()(value_expression_reference const& ref) const;
        std::string operator()(submember const& ref) const;
        std::string operator()(void_type const&) const;
        std::string operator()(thistype const&) const;
        std::string operator()(numeric_literal_reference const&) const;
        std::string operator()(auto_temploidic const&) const;
        std::string operator()(type_temploidic const&) const;
        std::string operator()(temploid_reference const&) const;
        std::string operator()(function_arg const&) const;
        std::string operator()(nvalue_slot const&) const;
        std::string operator()(dvalue_slot const&) const;
        std::string operator()(procedure_type const&) const;
        std::string operator()(ptrref_type const&) const;
        std::string operator()(instanciation_reference const&) const;
        std::string operator()(string_literal_reference const&) const;
        std::string operator()(array_initializer_type const&) const;

      public:
        type_symbol_stringifier() = default;
    };

    std::string to_string(vmir2::invocation_args const& ref)
    {
        std::string result = "[";
        bool first = true;
        for (auto const& [name, arg] : ref.named)
        {
            if (first)
                first = false;
            else
                result += ", ";
            result += "@" + name + " " + std::to_string(arg);
        }
        for (auto const& arg : ref.positional)
        {
            if (first)
                first = false;
            else
                result += ", ";
            result += std::to_string(arg);
        }
        result += "]";
        return result;
    }
    std::string to_string(codegen_invocation_args const& ref)
    {
        std::string result = "[";
        bool first = true;
        for (auto const& [name, arg] : ref.named)
        {
            if (first)
                first = false;
            else
                result += ", ";
            result += "@" + name + " " + std::to_string(arg);
        }
        for (auto const& arg : ref.positional)
        {
            if (first)
                first = false;
            else
                result += ", ";
            result += std::to_string(arg);
        }
        result += "]";
        return result;
    }

    bool is_template(type_symbol const& ref);

    std::string to_string(expression const& expr);

    struct is_template_visitor
    {
      public:
        is_template_visitor()
        {
        }

        bool operator()(storage const& ref) const
        {
            for (auto const& storable_type : ref.storable_types)
            {
                if (is_template(storable_type))
                {
                    return true;
                }
            }
            return false;
        }

        bool operator()(aligned_storage const& ref) const
        {
            return false;
        }

        bool operator()(array_initializer_type const& ref) const
        {
            return false;
        }

        bool operator()(procedure_type const& ref) const
        {
            for (auto const& [name, arg] : ref.signature.params.named)
            {
                if (is_template(arg))
                {
                    return true;
                }
            }

            for (auto const& arg : ref.signature.params.positional)
            {
                if (is_template(arg))
                {
                    return true;
                }
            }

            if (ref.signature.return_type.has_value() && is_template(*ref.signature.return_type))
            {
                return true;
            }

            return false;
        }


        bool operator()(readonly_constant const& ref) const
        {
            return false;
        }

        bool operator()(byte_type const& ref) const
        {
            return false;
        }

        bool operator()(initguard_type const&) const
        {
            return false;
        }

        bool operator()(initguard_lock_type const&) const
        {
            return false;
        }

        bool operator()(string_literal_reference const&) const
        {
            return false;
        }

        bool operator()(subsymbol const& ref) const
        {
            return is_template(ref.of);
        }

        bool operator()(size_type const&)
        {
            return false;
        }

        bool operator()(freebound_identifier const& ref) const
        {
            return false; // Freebound identifiers are not templates
        }

        bool operator()(instanciation_reference const& type)
        {
            for (auto& p : type.params.positional)
            {
                if (is_template(p))
                    return true;
            }
            for (auto& p : type.params.named)
            {
                if (is_template(p.second))
                    return true;
            }
            return is_template(type.temploid);
        }

        bool operator()(array_type const& ref) const
        {
            return is_template(ref.element_type);
        }

        bool operator()(ptrref_type const& ref) const
        {
            switch (ref.qual)
            {
            case qualifier::auto_:
            case qualifier::output:
            case qualifier::input:
                return true;
            case qualifier::mut:
            case qualifier::constant:
            case qualifier::write:
            case qualifier::temp:
                break;
            }
            return is_template(ref.target);
        }

        bool operator()(nvalue_slot const&) const
        {
            return false;
        }

        bool operator()(initialization_reference const& ref) const
        {
            if (is_template(ref.initializee))
                return true;
            for (auto& p : ref.parameters.positional)
            {
                if (is_template(p))
                    return true;
            }
            // TODO: Support named parameters here
            return false;
        }

        bool operator()(temploid_reference const& ref) const
        {
            if (is_template(ref.templexoid))
                return true;
            return false;
        }

        bool operator()(context_reference const& ref) const
        {
            return false;
        }

        bool operator()(attached_type_reference const& ref) const
        {
            return is_template(ref.carrying_type) || is_template(ref.attached_symbol);
        }

        bool operator()(int_type const& ref) const
        {
            return false;
        }

        bool operator()(bool_type const& ref) const
        {
            return false;
        }

        bool operator()(value_expression_reference const& ref) const
        {
            return false;
        }

        bool operator()(void_type const&) const
        {
            return false;
        }

        bool operator()(absolute_module_reference const& ref) const
        {
            return false;
        }

        bool operator()(submember const& ref) const
        {
            return is_template(ref.of);
        }

        bool operator()(numeric_literal_reference const&) const
        {
            return false;
        }

        bool operator()(auto_temploidic const&) const
        {
            return true;
        }

        bool operator()(type_temploidic const&) const
        {
            return true;
        }

        bool operator()(dvalue_slot const& t) const
        {
            return is_template(t.target);
        }

        bool operator()(thistype const& t) const
        {
            // TODO: is this correct?
            return false;
        }
    };

    bool is_template(type_symbol const& ref)
    {
        return rpnx::apply_visitor< bool >(ref, is_template_visitor{});
    }

    std::optional< type_symbol > type_parent(type_symbol input)
    {
        if (input.template type_is< subsymbol >())
        {
            return as< subsymbol >(input).of;
        }
        else if (input.template type_is< ptrref_type >())
        {
            return as< ptrref_type >(input).target;
        }
        else if (input.template type_is< initialization_reference >())
        {
            return as< initialization_reference >(input).initializee;
        }
        else if (input.template type_is< instanciation_reference >())
        {
            return as< instanciation_reference >(input).temploid;
        }
        else if (input.template type_is< temploid_reference >())
        {
            return as< temploid_reference >(input).templexoid;
        }
        else if (input.template type_is< submember >())
        {
            return as< submember >(input).of;
        }
        else
        {
            return std::nullopt;
        }
    }

    type_symbol with_context(type_symbol const& ref, type_symbol const& context)
    {
        if (ref.template type_is< context_reference >())
        {
            return context;
        }
        else if (ref.template type_is< storage >())
        {
            storage output;
            for (auto const& stored_type : as< storage >(ref).storable_types)
            {
                output.storable_types.insert(with_context(stored_type, context));
            }
            return output;
        }
        else if (ref.template type_is< subsymbol >())
        {
            return subsymbol{with_context(as< subsymbol >(ref).of, context), as< subsymbol >(ref).name};
        }
        else if (ref.template type_is< ptrref_type >())
        {
            ptrref_type copy = as< ptrref_type >(ref);
            copy.target = with_context(as< ptrref_type >(ref).target, context);
            return copy;
        }
        else if (ref.template type_is< procedure_type >())
        {
            procedure_type copy = as< procedure_type >(ref);
            for (auto& [name, param] : copy.signature.params.named)
            {
                param = with_context(param, context);
            }
            for (auto& param : copy.signature.params.positional)
            {
                param = with_context(param, context);
            }
            if (copy.signature.return_type.has_value())
            {
                copy.signature.return_type = with_context(*copy.signature.return_type, context);
            }
            return copy;
        }
        else if (ref.template type_is< initialization_reference >())
        {
            initialization_reference output = as< initialization_reference >(ref);
            output.initializee = with_context(output.initializee, context);
            for (auto& p : output.parameters.positional)
            {
                p = with_context(p, context);
            }
            return output;
        }
        else
        {
            return ref;
        }
    }

    std::string type_symbol_stringifier::operator()(subsymbol const& ref) const
    {
        return to_string(ref.of) + "::" + ref.name;
    }

    std::string type_symbol_stringifier::operator()(size_type const& ref) const
    {
        return "SZ";
    }

    std::string type_symbol_stringifier::operator()(array_type const& arr) const
    {
        return "[" + to_string(arr.element_count) + "] " + to_string(arr.element_type);
    }

    std::string type_symbol_stringifier::operator()(ptrref_type const& ref) const
    {
        std::string output;

        switch (ref.qual)
        {
        case qualifier::auto_:
            output += "AUTO";
            break;
        case qualifier::constant:
            output += "CONST";
            break;
        case qualifier::write:
            output += "WRITE";
            break;
        case qualifier::mut:
            output += "MUT";
            // the default, so no need to add anything
            break;
        case qualifier::temp:
            output += "TEMP";
            break;
        case qualifier::input:
            output += "INPUT";
            break;
        case qualifier::output:
            output += "OUTPUT";
            break;
        }

        switch (ref.ptr_class)
        {
        case pointer_class::instance:
            output += "->";
            break;
        case pointer_class::array:
            output += "=>>";
            break;
        case pointer_class::machine:
            output += "*";
            break;
        case pointer_class::ref:
            output += "&";
        }

        output += " #{ ";

        output += rpnx::apply_visitor< std::string >(ref.target, *this);

        output += " }";

        return output;
    }
    std::string type_symbol_stringifier::operator()(procedure_type const& ref) const
    {
        std::string output = "PROCEDURE";

        if (ref.calling_convention != "DEFAULT")
        {
            output += " " + ref.calling_convention;
        }
        if (ref.is_noexcept)
        {
            output += " NOEXCEPT";
        }

        output += "(";
        bool first = true;
        for (auto const& [name, arg] : ref.signature.params.named)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;
            output += "@" + name + " " + to_string(arg);
        }
        for (auto const& arg : ref.signature.params.positional)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;
            output += to_string(arg);
        }

        auto return_type = ref.signature.return_type.value_or(type_symbol(void_type{}));
        if (!typeis< void_type >(return_type))
        {
            output += ": " + to_string(return_type);
        }

        output += ")";
        return output;
    }
    std::string type_symbol_stringifier::operator()(initialization_reference const& ref) const
    {
        std::string output = rpnx::apply_visitor< std::string >(ref.initializee, *this);
        output += " #(";
        bool first = true;
        for (auto const& [name, type] : ref.parameters.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + name + " " + to_string(type);
        }
        for (auto& p : ref.parameters.positional)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += rpnx::apply_visitor< std::string >(p, *this);
        }
        output += ")";
        return output;
    }

    std::string type_symbol_stringifier::operator()(instanciation_reference const& ref) const
    {

        temploid_reference const& sel = ref.temploid;
        std::string output = rpnx::apply_visitor< std::string >(sel.templexoid, *this);

        output += " #{";

        // TODO: consider if keep this
        if (false) // (sel.which.builtin)
        {
            output += "BUILTIN; ";
        }
        bool first = true;
        for (auto const& [name, param] : sel.which.interface.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + name + " " + to_string(param.type);
            if (ref.params.named.at(name) != param.type)
            {
                output += ": " + to_string(ref.params.named.at(name));
            }
        }
        for (size_t i = 0; i < sel.which.interface.positional.size(); i++)
        {
            auto const& param = sel.which.interface.positional.at(i);
            if (first)
                first = false;
            else
                output += ", ";
            output += to_string(param.type);
            if (ref.params.positional.at(i) != sel.which.interface.positional.at(i).type)
            {
                output += ": " + to_string(ref.params.positional.at(i));
            }
        }
        output += "}";
        return output;
    }

    std::string type_symbol_stringifier::operator()(storage const& ref) const
    {
        std::string result = "STORAGE(";
        bool first = true;
        for (auto const& stored_type : ref.storable_types)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += ", ";
            }
            result += to_string(stored_type);
        }
        result += ")";
        return result;
    }

    std::string type_symbol_stringifier::operator()(aligned_storage const& ref) const
    {
        return "ALIGNED_STORAGE(" + to_string(ref.size) + ", " + to_string(ref.align) + ")";
    }
    std::string type_symbol_stringifier::operator()(readonly_constant const& ref) const
    {
        if (ref.kind == constant_kind::string)
        {
            return "STRING_CONSTANT";
        }
        else if (ref.kind == constant_kind::cstring)
        {
            return "CSTRING_CONSTANT";
        }
        else if (ref.kind == constant_kind::numeric)
        {
            return "NUMERIC_CONSTANT";
        }
        else if (ref.kind == constant_kind::data)
        {
            return "DATA_CONSTANT";
        }
        else
        {
            throw compiler_bug("Unknown constant kind");
        }
    }
    std::string type_symbol_stringifier::operator()(freebound_identifier const& ref) const
    {
        return ref.name;
    }
    std::string type_symbol_stringifier::operator()(context_reference const& ref) const
    {
        return "CONTEXT";
    }
    std::string type_symbol_stringifier::operator()(attached_type_reference const& ref) const
    {
        return "BINDING(" + to_string(ref.carrying_type) + ", " + to_string(ref.attached_symbol) + ")";
    }
    std::string type_symbol_stringifier::operator()(int_type const& ref) const
    {
        return (ref.has_sign ? "I" : "U") + std::to_string(ref.bits);
    }
    std::string type_symbol_stringifier::operator()(bool_type const& ref) const
    {
        return "BOOL";
    }
    std::string type_symbol_stringifier::operator()(byte_type const& ref) const
    {
        return "BYTE";
    }
    std::string type_symbol_stringifier::operator()(initguard_type const& ref) const
    {
        return "INITGUARD";
    }
    std::string type_symbol_stringifier::operator()(initguard_lock_type const& ref) const
    {
        return "INITGUARD_LOCK";
    }
    std::string type_symbol_stringifier::operator()(value_expression_reference const& ref) const
    {
        return "VALUE";
    }
    std::string type_symbol_stringifier::operator()(void_type const&) const
    {
        return "VOID";
    }

    std::string type_symbol_stringifier::operator()(thistype const&) const
    {
        return "THISTYPE";
    }
    std::string type_symbol_stringifier::operator()(absolute_module_reference const& ref) const
    {
        return "MODULE(" + ref.module_name + ")";
    }
    std::string type_symbol_stringifier::operator()(submember const& ref) const
    {
        return to_string(ref.of) + "::." + ref.name;
    }

    std::string type_symbol_stringifier::operator()(numeric_literal_reference const&) const
    {
        return "NUMERIC_LITERAL";
    }

    std::string type_symbol_stringifier::operator()(string_literal_reference const&) const
    {
        return "STRING_LITERAL";
    }

    std::string type_symbol_stringifier::operator()(array_initializer_type const& ai) const
    {
        return "__ARRAY_INITIALIZER( " + to_string(ai.element_type) + ", " + std::to_string(ai.count) + " )";
    }

    std::string type_symbol_stringifier::operator()(auto_temploidic const& val) const
    {
        return "AUTO(" + val.name + ")";
    }
    std::string type_symbol_stringifier::operator()(type_temploidic const& val) const
    {
        return "TT(" + val.name + ")";
    }

    std::string type_symbol_stringifier::operator()(function_arg const& ref) const
    {
        return "TODO";
        // TODO: implement this
    }

    std::string type_symbol_stringifier::operator()(nvalue_slot const& ref) const
    {
        return "NEW{ " + to_string(ref.target) + " }";
    }

    std::string type_symbol_stringifier::operator()(dvalue_slot const& ref) const
    {
        return "DESTROY{ " + to_string(ref.target) + "}";
    }

    std::string type_symbol_stringifier::operator()(temploid_reference const& ref) const
    {
        std::string output = rpnx::apply_visitor< std::string >(ref.templexoid, *this) + "#[";
        bool first = true;
        if (false) // ref.which.builtin)
        {
            output += "BUILTIN; ";
        }
        for (auto const& arg : ref.which.interface.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + arg.first + " " + to_string(arg.second.type);
        }
        for (auto const& arg : ref.which.interface.positional)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += to_string(arg);
        }
        output += "]";
        return output;
    }

    std::optional< template_match_results > match_template_noconv2(type_symbol const& template_type, type_symbol const& type);

    std::optional< template_match_results > match_template_noconv(type_symbol const& template_type, type_symbol const& type)
    {
        std::optional< template_match_results > results;
        assert(is_canonical(type));

        // Template portion match.
        // e.g. T(t1) matches I32 and sets t1 to I32
        if (typeis< auto_temploidic >(template_type))
        {
            template_match_results output;
            auto name = as< auto_temploidic >(template_type).name;
            if (!name.empty())
            {
                output.matches[name] = type;
            }
            output.type = type;
            results = std::move(output);
            return results;
        }

        if (is_auto_ref(template_type))
        {
            // Matches any reference type
            ptrref_type const& template_autoref = as< ptrref_type >(template_type);

            if (!is_ref(type))
            {
                return std::nullopt;
            }

            auto type_refof = remove_ref(type);

            auto match = match_template_noconv(template_autoref.target, type_refof);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = type;
            return match;
        }

        // Exact match, e.g. I32 matches I32
        if (template_type == type)
        {
            template_match_results result{};
            result.type = type;
            return result;
        }

        // Impossible to match in this case, e.g. a pointer reference cannot match an integer
        // Note that template matching doesn't do any implicit conversions.
        // So for example, MUT& I32 cannot match a template CONST& I32,
        // even though a MUT& I32 can be implicitly converted to a CONST& I32.
        // This is because might be matching a template parameter, and implementations might differ,
        // so we can't assume that the conversion is valid.
        // For example, a structure might implement differently depending on the mutability of the reference.
        // Such as including a mutex in the MUT& version, but not in the CONST& version.
        // e.g. foo#(I32, MUT& I32) cannot match foo#(I32, CONST& I32)

        // TODO: Add template_arg_matches to the top level to allow builtin implicit conversions

        if (typeis< ptrref_type >(template_type))
        {
            auto const& ptr_template = as< ptrref_type >(template_type);

            if (!type.type_is< ptrref_type >())
            {
                return std::nullopt;
            }
            auto const& ptr_type = as< ptrref_type >(type);

            if (ptr_template.ptr_class != ptr_type.ptr_class)
            {
                return std::nullopt;
            }

            auto qual_match = qualifier_template_match(ptr_template.qual, ptr_type.qual);

            if (!qual_match)
            {
                return std::nullopt;
            }

            auto match = match_template_noconv(ptr_template.target, ptr_type.target);
            if (!match.has_value())
            {
                return std::nullopt;
            }

            match->type = ptrref_type{
                .target = std::move(match->type),
                .ptr_class = ptr_type.ptr_class,
                .qual = *qual_match,
            };
            return match;
        }
        else if (typeis< subsymbol >(template_type))
        {
            auto const& sub_template = as< subsymbol >(template_type);
            if (!type.type_is< subsymbol >())
            {
                return std::nullopt;
            }
            auto const& sub_type = as< subsymbol >(type);
            if (sub_template.name != sub_type.name)
            {
                return std::nullopt;
            }
            auto match = match_template(sub_template.of, sub_type.of);
            if (!match.has_value())
            {
                return std::nullopt;
            }
            match->type = subsymbol{std::move(match->type), sub_template.name};
            return match;
        }
        else if (typeis< submember >(template_type))
        {
            // it is not possible for a type to be a subdotentity_reference
            throw std::logic_error("::. cannot appear in an argument type or argument type template");
        }
        else if (typeis< initialization_reference >(template_type))
        {
            initialization_reference const& template_funct = as< initialization_reference >(template_type);
            initialization_reference const& type_funct = as< initialization_reference >(type);

            if (template_funct.parameters.positional.size() != type_funct.parameters.positional.size())
            {
                return std::nullopt;
            }

            // TODO: support named parameters

            auto callee_match = match_template(template_funct.initializee, type_funct.initializee);
            if (!callee_match)
            {
                return std::nullopt;
            }

            template_match_results all_results;
            all_results.type = std::move(callee_match->type);
            all_results.matches = std::move(callee_match->matches);

            std::vector< type_symbol > instanciated_parameters;

            // TODO: here
            // rpnx::unimplemented();
        }

        if (template_type.type() != type.type())
        {
            return std::nullopt;
        }

        // In other cases, we are talking about a non-composite reference
        // However, we should make sure we don't miss types

        assert(typeis< int_type >(template_type) || typeis< bool_type >(template_type) || typeis< void_type >(template_type) || typeis< absolute_module_reference >(template_type) || typeis< numeric_literal_reference >(template_type));
        return std::nullopt;
    }

    std::optional< template_match_results > match_template(type_symbol const& template_type, type_symbol const& type)
    {
        std::optional< template_match_results > results;
        assert(is_canonical(type));

        // Template portion match.
        // e.g. AUTO(t1) matches I32 and sets t1 to I32
        if (typeis< auto_temploidic >(template_type))
        {
            template_match_results output;
            auto name = as< auto_temploidic >(template_type).name;

            type_symbol const* type_target = nullptr;
            if (type.type_is< ptrref_type >() && type.as< ptrref_type >().ptr_class == pointer_class::ref)
            {
                type_target = &as< ptrref_type >(type).target;
            }
            else
            {
                type_target = &type;
            }

            if (!name.empty())
            {
                output.matches[name] = *type_target;
            }
            output.type = *type_target;
            results = std::move(output);
            return results;
        }

        if (typeis< type_temploidic >(template_type))
        {
            template_match_results output;
            auto name = as< type_temploidic >(template_type).name;

            if (!name.empty())
            {
                output.matches[name] = type;
            }
            output.type = type;
            results = std::move(output);
            return results;
        }

        if (is_auto_ref(template_type))
        {
            // Matches any reference type
            ptrref_type const& template_autoref = as< ptrref_type >(template_type);

            if (!is_ref(type))
            {
                return std::nullopt;
            }

            auto type_refof = remove_ref(type);

            auto match = match_template(template_autoref.target, type_refof);
            if (!match)
            {
                return std::nullopt;
            }
            match->type = type;
            return match;
        }

        // Exact match, e.g. I32 matches I32
        if (template_type == type)
        {
            template_match_results result{};
            result.type = type;
            return result;
        }

        // Impossible to match in this case, e.g. a pointer reference cannot match an integer
        // Note that template matching doesn't do any implicit conversions.
        // So for example, MUT& I32 cannot match a template CONST& I32,
        // even though a MUT& I32 can be implicitly converted to a CONST& I32.
        // This is because might be matching a template parameter, and implementations might differ,
        // so we can't assume that the conversion is valid.
        // For example, a structure might implement differently depending on the mutability of the reference.
        // Such as including a mutex in the MUT& version, but not in the CONST& version.
        // e.g. foo#(I32, MUT& I32) cannot match foo#(I32, CONST& I32)

        // TODO: Add template_arg_matches to the top level to allow builtin implicit conversions

        if (typeis< ptrref_type >(template_type))
        {
            auto const& ptr_template = as< ptrref_type >(template_type);

            if (!type.type_is< ptrref_type >())
            {
                return std::nullopt;
            }
            auto const& ptr_type = as< ptrref_type >(type);

            if (ptr_template.ptr_class != ptr_type.ptr_class)
            {
                return std::nullopt;
            }

            auto qual_match = qualifier_template_match(ptr_template.qual, ptr_type.qual);

            if (!qual_match)
            {
                return std::nullopt;
            }

            auto match = match_template(ptr_template.target, ptr_type.target);
            if (!match.has_value())
            {
                return std::nullopt;
            }

            match->type = ptrref_type{
                .target = std::move(match->type),
                .ptr_class = ptr_type.ptr_class,
                .qual = *qual_match,
            };
            return match;
        }
        else if (typeis< subsymbol >(template_type))
        {
            auto const& sub_template = as< subsymbol >(template_type);
            if (!type.type_is< subsymbol >())
            {
                return std::nullopt;
            }
            auto const& sub_type = as< subsymbol >(type);
            if (sub_template.name != sub_type.name)
            {
                return std::nullopt;
            }
            auto match = match_template(sub_template.of, sub_type.of);
            if (!match.has_value())
            {
                return std::nullopt;
            }
            match->type = subsymbol{std::move(match->type), sub_template.name};
            return match;
        }
        else if (typeis< submember >(template_type))
        {
            // it is not possible for a type to be a subdotentity_reference
            throw std::logic_error("::. cannot appear in an argument type or argument type template");
        }
        else if (typeis< initialization_reference >(template_type))
        {
            initialization_reference const& template_funct = as< initialization_reference >(template_type);
            initialization_reference const& type_funct = as< initialization_reference >(type);

            if (template_funct.parameters.positional.size() != type_funct.parameters.positional.size())
            {
                return std::nullopt;
            }

            // TODO: support named parameters

            auto callee_match = match_template(template_funct.initializee, type_funct.initializee);
            if (!callee_match)
            {
                return std::nullopt;
            }

            template_match_results all_results;
            all_results.type = std::move(callee_match->type);
            all_results.matches = std::move(callee_match->matches);

            std::vector< type_symbol > instanciated_parameters;

            // TODO: here
            // rpnx::unimplemented();
        }

        if (template_type.type() != type.type())
        {
            return std::nullopt;
        }

        // In other cases, we are talking about a non-composite reference
        // However, we should make sure we don't miss types

        std::string template_type_str = quxlang::to_string(template_type);
        std::string type_str = quxlang::to_string(type);
        assert(typeis< int_type >(template_type) || typeis< bool_type >(template_type) || typeis< void_type >(template_type) || typeis< absolute_module_reference >(template_type) || typeis< numeric_literal_reference >(template_type));
        return std::nullopt;
    }

    std::string to_string(argif const& arg)
    {
        std::string output;
        output += to_string(arg.type);
        if (arg.is_defaulted)
        {
            output += " DEFAULTED";
        }
        return output;
    }

    std::string to_string(invotype const& ref)
    {
        std::string output;
        bool first = true;
        output += "INVOTYPE(";
        for (auto const& [name, arg] : ref.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + name + " " + to_string(arg);
        }
        for (auto const& arg : ref.positional)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += to_string(arg);
        }
        output += ")";
        return output;
    }

    std::string to_string(intertype const& ref)
    {
        std::string output;
        bool first = true;
        output += "INTERTYPE(";
        for (auto const& [name, arg] : ref.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + name + " " + to_string(arg);
        }
        for (auto const& arg : ref.positional)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += to_string(arg);
        }
        output += ")";
        return output;
    }

    std::string to_string(vmir2::routine_parameters const& ref)
    {
        std::string output;
        bool first = true;
        output += "PARAMETERS(";
        for (auto const& [name, arg] : ref.named)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += "@" + name + " " + to_string(arg.type);
        }
        for (auto const& arg : ref.positional)
        {
            if (first)
                first = false;
            else
                output += ", ";
            output += to_string(arg.type);
        }
        output += ")";
        return output;
    }

    type_symbol get_templexoid(initialization_reference const& ref)
    {
        auto callee = ref.initializee;

        assert(callee.type_is< temploid_reference >());

        return as< temploid_reference >(callee).templexoid;
    }

    std::optional< type_symbol > func_class(type_symbol const& func)
    {
        std::string func_str = to_string(func);
        auto tmplx = func.get_as< instanciation_reference >().temploid.templexoid;
        if (tmplx.type_is< submember >())
        {
            return as< submember >(tmplx).of;
        }
        return std::nullopt;
    }
    std::optional< qualifier > qualifier_template_match(qualifier to_qual, qualifier from_qual)
    {

        // Only non-template qualifiers are allowed as the match type
        assert(from_qual != qualifier::auto_);
        assert(from_qual != qualifier::input);
        assert(from_qual != qualifier::output);

        switch (to_qual)
        {
        case qualifier::auto_: {
            // AUTO can match all other qualifiers
            return from_qual;
        }
        case qualifier::constant: {
            // Matches everything except WRITE as CONST
            if (from_qual == qualifier::write)
            {
                return std::nullopt;
            }
            return qualifier::constant;
        }
        case qualifier::mut: {
            // Matches MUT only
            if (from_qual == qualifier::mut)
            {
                return qualifier::mut;
            }
            return std::nullopt;
        }
        case qualifier::temp: {
            // Matches TEMP only
            if (from_qual == qualifier::temp)
            {
                return qualifier::temp;
            }
            return std::nullopt;
        }
        case qualifier::input: {
            // INPUT is a template that matches as either TEMP or CONST
            // TODO: Consider allowing DESTROY to match as an INPUT?
            if (from_qual == qualifier::temp)
            {
                return qualifier::temp;
            }
            else
            {
                return qualifier_template_match(qualifier::constant, from_qual);
            }
        }
        case qualifier::output: {
            // This is a template that matches new and write, but for
            // now NEW isn't implemented on pointers
            // TODO: refactor NEW/DESTROY types to use qualifier
            return qualifier_template_match(qualifier::write, from_qual);
        }
        case qualifier::write: {
            // Write matches anything writable, but not constant.
            // Note that we exclude TEMP as well since that is a "discarded value" so writes to a temp
            // may be considered meaningless.
            if (from_qual == qualifier::temp || from_qual == qualifier::constant)
            {
                return std::nullopt;
            }
            return from_qual;
            break;
        }
        }

        throw compiler_bug("should be unreachable");
    }
    std::optional< pointer_class > pointer_class_template_match(pointer_class template_class, pointer_class match_class)
    {
        switch (template_class)
        {
        case pointer_class::instance:
            return pointer_class::instance;
        case pointer_class::array:
            if (match_class == pointer_class::instance)
            {
                return std::nullopt;
            }
            return pointer_class::array;
        case pointer_class::machine:
            if (match_class == pointer_class::machine)
            {
                return pointer_class::machine;
            }
            return std::nullopt;
        case pointer_class::ref:
            if (match_class == pointer_class::ref)
            {
                return pointer_class::ref;
            }
            return std::nullopt;
        }

        throw compiler_bug("should be unreachable");
    }

} // namespace quxlang

namespace quxlang
{
    namespace
    {
        class template_matcher
        {
          private:
            quxlang::template_match_results results;

          public:
            template_match_results take_results()
            {
                return std::move(results);
            }
            bool check(invotype template_ct, invotype match_ct, bool conv);
            bool check(intertype template_ct, intertype match_ct, bool conv);
            bool check(type_symbol template_val, type_symbol match_val, bool conv);

            bool check_impl(byte_type, byte_type, bool conv)
            {
                return true;
            }

            bool check_impl(initguard_type const&, initguard_type const&, bool conv)
            {
                return true;
            }

            bool check_impl(initguard_lock_type const&, initguard_lock_type const&, bool conv)
            {
                return true;
            }

            bool check_impl(auto_temploidic const& template_val, auto_temploidic const& match_val, bool conv)
            {
                throw compiler_bug("should be unreachable");
            }

            bool check_impl(storage const& template_val, storage const& match_val, bool conv)
            {
                if (template_val.storable_types.size() != match_val.storable_types.size())
                {
                    return false;
                }
                auto template_it = template_val.storable_types.begin();
                auto match_it = match_val.storable_types.begin();
                for (; template_it != template_val.storable_types.end() && match_it != match_val.storable_types.end(); ++template_it, ++match_it)
                {
                    if (!check(*template_it, *match_it, conv))
                    {
                        return false;
                    }
                }
                return true;
            }

            bool check_impl(aligned_storage const& template_val, aligned_storage const& match_val, bool conv)
            {
                return template_val.size == match_val.size && template_val.align == match_val.align;
            }

            bool check_impl(type_temploidic const& template_val, type_temploidic const& match_val, bool conv)
            {
                throw compiler_bug("should be unreachable");
            }

            bool check_impl(readonly_constant const& template_val, readonly_constant const& match_val, bool conv)
            {
                return template_val.kind == match_val.kind;
            }

            bool check_impl(subsymbol const& template_val, subsymbol const& match_val, bool conv)
            {
                if (template_val.name != match_val.name)
                {
                    return false;
                }
                return check(template_val.of, match_val.of, false);
            }

            bool check_impl(freebound_identifier const& template_val, freebound_identifier const& match_val, bool conv)
            {
                throw compiler_bug("can't use a freebound identifier as a template parameter");
            }

            bool check_impl(ptrref_type const& template_val, ptrref_type const& match_val, bool conv)
            {
                if (template_val.qual != match_val.qual && !conv && qualifier_template_match(template_val.qual, match_val.qual) == std::nullopt)
                {
                    return false;
                }

                return check(template_val.target, match_val.target, true);
            }

            bool check_impl(size_type const& template_val, size_type const& match_val, bool conv)
            {
                return true;
            }

            bool check_impl(array_initializer_type const& template_val, array_initializer_type const& match_val, bool conv)
            {
                throw compiler_bug("should be unreachable");
            }
            bool check_impl(array_type const& template_val, array_type const& match_val, bool conv)
            {
                // TODO: Allow match expressions against element count
                // This may require lookup expressions
                // currently this is not possible because the check occurs outside the compiler context.

                if (template_val.element_count != match_val.element_count)
                {
                    return false;
                }

                return check(template_val.element_type, match_val.element_type, true);
            }

            bool check_impl(submember const& tmpl, submember const& val, bool conv)
            {
                if (tmpl.name != val.name)
                {
                    return false;
                }

                return check(tmpl.of, val.of, conv);
            }

            bool check_impl(instanciation_reference const& tmpl, instanciation_reference const& val, bool conv)
            {
                if (!check(tmpl.temploid, val.temploid, conv))
                {
                    return false;
                }

                if (tmpl.params.named.size() != val.params.named.size())
                {
                    return false;
                }

                for (auto const& [name, type] : tmpl.params.named)
                {
                    if (!check(type, val.params.named.at(name), conv))
                    {
                        return false;
                    }
                }

                if (tmpl.params.positional.size() != val.params.positional.size())
                {
                    return false;
                }

                for (size_t i = 0; i < tmpl.params.positional.size(); i++)
                {
                    if (!check(tmpl.params.positional[i], val.params.positional[i], conv))
                    {
                        return false;
                    }
                }

                return true;
            }

            bool check_impl(initialization_reference const& tmpl, initialization_reference const& val, bool conv)
            {
                bool of_match = check(tmpl.initializee, val.initializee, conv);

                if (!of_match)
                    return false;

                return check(tmpl.parameters, val.parameters, false);
            }

            bool check_impl(void_type const&, void_type const&, bool conv)
            {
                return true;
            }

            bool check_impl(quxlang::int_type const& tmpl, quxlang::int_type const& val, bool conv)
            {
                return tmpl == val;
            }

            bool check_impl(quxlang::bool_type const&, quxlang::bool_type const&, bool conv)
            {
                return true;
            }

            bool check_impl(quxlang::absolute_module_reference const& tmpl, quxlang::absolute_module_reference const& val, bool conv)
            {
                return tmpl == val;
            }

            bool check_impl(context_reference const&, context_reference const&, bool conv)
            {
                throw compiler_bug("should be unreachable");
            }

            bool check_impl(temploid_reference const& tmpl, temploid_reference const& val, bool conv)
            {
                if (!check(tmpl.templexoid, val.templexoid, false))
                {
                    return false;
                }

                if (!check(tmpl.which.interface, val.which.interface, false))
                {
                    return false;
                }

                return true;
            }

            bool check_impl(value_expression_reference const&, value_expression_reference const&, bool conv)
            {
                throw rpnx::unimplemented();
            }

            bool check_impl(thistype const&, thistype const&, bool conv)
            {
                // TOOD: Match against values for "this"
                return true;
            }

            bool check_impl(attached_type_reference const& tmpl, attached_type_reference const& val, bool conv)
            {
                if (!check(tmpl.attached_symbol, val.attached_symbol, false))
                {
                    return false;
                }

                return check(tmpl.carrying_type, val.carrying_type, false);
            }

            bool check_impl(numeric_literal_reference const& tmpl, numeric_literal_reference const& val, bool conv)
            {
                return true;
            }

            bool check_impl(string_literal_reference const& tmpl, string_literal_reference const& val, bool conv)
            {
                return true;
            }

            bool check_impl(nvalue_slot const& tmpl, nvalue_slot const& val, bool conv)
            {
                return check(tmpl.target, val.target, false);
            }

            bool check_impl(dvalue_slot const& tmpl, dvalue_slot const& val, bool conv)
            {
                return check(tmpl.target, val.target, false);
            }

            bool check_impl(procedure_type const& tmpl, procedure_type const& val, bool conv)
            {
                if (tmpl.calling_convention != val.calling_convention)
                {
                    return false;
                }

                if (tmpl.is_noexcept != val.is_noexcept)
                {
                    return false;
                }

                if (!check(tmpl.signature.params, val.signature.params, false))
                {
                    return false;
                }

                auto tmpl_ret = tmpl.signature.return_type.value_or(type_symbol(void_type{}));
                auto val_ret = val.signature.return_type.value_or(type_symbol(void_type{}));
                return check(tmpl_ret, val_ret, false);
            }
        };
        bool template_matcher::check(invotype template_ct, invotype match_ct, bool conv)
        {
            if (template_ct.named.size() != match_ct.named.size())
            {
                return false;
            }

            for (auto const& [name, type] : template_ct.named)
            {
                if (match_ct.named.find(name) == match_ct.named.end())
                {
                    return false;
                }

                if (!check(type, match_ct.named.at(name), conv))
                {
                    return false;
                }
            }

            if (template_ct.positional.size() != match_ct.positional.size())
            {
                return false;
            }

            for (size_t i = 0; i < template_ct.positional.size(); i++)
            {
                if (!check(template_ct.positional[i], match_ct.positional[i], conv))
                {
                    return false;
                }
            }

            return true;
        }

        bool template_matcher::check(intertype template_ct, intertype match_ct, bool conv)
        {
            if (template_ct.named.size() != match_ct.named.size())
            {
                return false;
            }

            for (auto const& [name, type] : template_ct.named)
            {
                if (match_ct.named.find(name) == match_ct.named.end())
                {
                    return false;
                }

                if (type.is_defaulted != match_ct.named.at(name).is_defaulted)
                {
                    return false;
                }

                if (!check(type.type, match_ct.named.at(name).type, conv))
                {
                    return false;
                }
            }

            if (template_ct.positional.size() != match_ct.positional.size())
            {
                return false;
            }

            for (size_t i = 0; i < template_ct.positional.size(); i++)
            {
                if (!check(template_ct.positional[i].type, match_ct.positional[i].type, conv))
                {
                    return false;
                }

                if (template_ct.positional[i].is_defaulted != match_ct.positional[i].is_defaulted)
                {
                    return false;
                }
            }

            return true;
        }
        bool template_matcher::check(type_symbol template_val, type_symbol match_val, bool conv)
        {
            std::string template_str = to_string(template_val);
            std::string match_str = to_string(match_val);

            if (typeis< auto_temploidic >(template_val))
            {
                auto const& template_ref = as< auto_temploidic >(template_val);

                if (results.matches.find(template_ref.name) != results.matches.end())
                {
                    return results.matches[template_ref.name] == match_val;
                }
                else
                {
                    results.matches[template_ref.name] = match_val;
                    return true;
                }
            }
            if (typeis< type_temploidic >(template_val))
            {
                auto const& template_ref = as< type_temploidic >(template_val);

                if (results.matches.find(template_ref.name) != results.matches.end())
                {
                    return results.matches[template_ref.name] == match_val;
                }
                else
                {
                    results.matches[template_ref.name] = match_val;
                    return true;
                }
            }

            if (is_auto_ref(template_val))
            {
                if (is_ref(match_val))
                {
                    return true;
                }
            }

            if (template_val.type() != match_val.type())
            {
                return false;
            }

            return rpnx::apply_visitor< bool >(
                template_val,
                [&](auto const& template_unwrapped)
                {
                    using val_type = std::decay_t< decltype(template_unwrapped) >;

                    val_type const& match_val_unwrapped = as< val_type >(match_val);

                    bool result = check_impl(template_unwrapped, match_val_unwrapped, conv);
                    if (result == false)
                    {
                        int x = 0;
                    }

                    return result;
                });
        }
    } // namespace

} // namespace quxlang

std::optional< quxlang::template_match_results > quxlang::match_template_noconv2(type_symbol const& template_type, type_symbol const& type)
{
    template_matcher matcher;
    if (matcher.check(template_type, type, false))
    {
        return matcher.take_results();
    }
    return std::nullopt;
}

std::optional< quxlang::template_match_results > quxlang::match_template2(type_symbol const& template_type, type_symbol const& type)
{
    template_matcher matcher;
    if (matcher.check(template_type, type, true))
    {
        return matcher.take_results();
    }
    return std::nullopt;
}

std::string quxlang::to_string(quxlang::type_symbol const& ref)
{
    return rpnx::apply_visitor< std::string >(ref, quxlang::type_symbol_stringifier{});
}

std::string quxlang::to_string(expression const& expr)
{
    auto str = rpnx::apply_visitor< std::string, rpnx::dispatch_type::branching >(expr, expression_stringifier{});

    // TODO: replace all "  " with " "
    return str;
}
