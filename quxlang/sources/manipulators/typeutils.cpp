// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/manipulators/typeutils.hpp"
#include <quxlang/cpu_attributes.hpp>
#include <quxlang/data/compilation_result.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/data/codegen_types.hpp"
#include "quxlang/data/constexpr_types.hpp"
#include "quxlang/data/function_statement.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/manipulators/expression_stringifier.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/unimplemented.hpp"
#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    auto conversion_mode_keyword(conversion_mode mode) -> std::string_view
    {
        switch (mode)
        {
        case conversion_mode::explicit_:
            return "EXPLICIT";
        case conversion_mode::reinterpret_:
            return "REINTERPRET";
        case conversion_mode::partial:
            return "PARTIAL";
        case conversion_mode::assume:
            return "ASSUME";
        case conversion_mode::checked:
            return "CHECKED";
        case conversion_mode::approximate:
            return "APPROXIMATE";
        case conversion_mode::dynamic_:
            return "DYNAMIC";
        }
        throw compiler_bug("Invalid conversion mode");
    }

    auto conversion_mode_from_keyword(std::string_view keyword) -> std::optional< conversion_mode >
    {
        for (conversion_mode mode : {conversion_mode::explicit_, conversion_mode::reinterpret_, conversion_mode::partial, conversion_mode::assume, conversion_mode::checked, conversion_mode::approximate, conversion_mode::dynamic_})
        {
            if (conversion_mode_keyword(mode) == keyword)
            {
                return mode;
            }
        }
        return std::nullopt;
    }

    auto atomic_type_argument(type_symbol const& type) -> std::optional< type_symbol >
    {
        if (!type.type_is< instanciation_reference >())
        {
            return std::nullopt;
        }

        instanciation_reference const& inst = type.get_as< instanciation_reference >();
        if (!inst.temploid.templexoid.type_is< builtin_symbol >())
        {
            return std::nullopt;
        }
        if (!is_builtin_atomic_templex_name(inst.temploid.templexoid.get_as< builtin_symbol >().name))
        {
            return std::nullopt;
        }
        auto type_arg = inst.params.named.find("T");
        if (type_arg == inst.params.named.end() || inst.params.named.size() != 1 || !inst.params.positional.empty())
        {
            return std::nullopt;
        }
        return parameter_instantiation_type(type_arg->second);
    }

    auto parameter_runtime_type(type_symbol const& parameter_type) -> std::optional< type_symbol >
    {
        if (typeis< void_type >(parameter_type) || typeis< numeric_literal_type >(parameter_type) || typeis< string_literal_type >(parameter_type))
        {
            return std::nullopt;
        }
        if (!typeis< attached_type_reference >(parameter_type))
        {
            return parameter_type;
        }

        attached_type_reference const& attached = as< attached_type_reference >(parameter_type);
        if (typeis< void_type >(attached.carrying_type))
        {
            return std::nullopt;
        }
        return attached.carrying_type;
    }

    std::string constexpr_parameter_value_to_string(type_symbol const& type, constexpr_value const& value);

    struct expression_stringifier
    {
        bool print_locations = false;

        expression_stringifier(bool print_locations = false) : print_locations(print_locations)
        {
        }

        std::string expr_to_string(expression const& expr) const
        {
            return to_string(expr, print_locations);
        }

        std::string operator()(expression_bits const& bits) const
        {
            return "BITS(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_bit const& bit_expression) const
        {
            return "BIT " + bit_expression.bit_index;
        }

        std::string operator()(expression_typecast const& cast) const
        {
            std::string result = "( " + expr_to_string(cast.expr) + " AS ";

            if (cast.mode)
            {
                result += conversion_mode_keyword(*cast.mode);
                result += " ";
            }

            result += to_string(cast.to_type) + " )";

            return result;
        }

        std::string operator()(expression_union_is const& expr) const
        {
            return "(" + expr_to_string(expr.subject) + " IS " + expr.option_name + ")";
        }

        std::string operator()(expression_variant_isa const& expr) const
        {
            return "(" + expr_to_string(expr.subject) + " ISA " + to_string(expr.type) + ")";
        }

        std::string operator()(expression_variant_unwrap const& expr) const
        {
            return "(UNWRAP " + expr_to_string(expr.subject) + " INTO " + to_string(expr.type) + ")";
        }

        std::string operator()(expression_pun const& expr) const
        {
            return "( PUN " + expr_to_string(expr.value) + " AS " + to_string(expr.as_type) + " )";
        }

        std::string operator()(expression_place const& expr) const
        {
            std::string result = "( PLACE AT(" + expr_to_string(expr.at) + ") " + to_string(expr.type);
            if (expr.assign_init)
            {
                result += " := " + expr_to_string(*expr.assign_init);
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
                    result += expr_to_string(expr.args[i].value);
                }
                result += ")";
            }
            result += " )";
            return result;
        }

        std::string operator()(expression_new const& expr) const
        {
            std::string result = "( NEW " + to_string(expr.type);
            rpnx::apply_visitor< void >(expr.initializer,
                [&](auto&& initializer)
                {
                    using initializer_type = std::decay_t< decltype(initializer) >;
                    if constexpr (std::is_same_v< initializer_type, new_from_initializer >)
                    {
                        result += " FROM ";
                        if (initializer.mode.has_value())
                        {
                            result += conversion_mode_keyword(*initializer.mode);
                            result += " ";
                        }
                        result += expr_to_string(initializer.source);
                    }
                    else if constexpr (std::is_same_v< initializer_type, new_arguments_initializer >)
                    {
                        result += ":(";
                        for (std::size_t i = 0; i < initializer.arguments.size(); ++i)
                        {
                            if (i != 0)
                            {
                                result += ", ";
                            }
                            if (initializer.arguments.at(i).name.has_value())
                            {
                                result += "@" + *initializer.arguments.at(i).name + " ";
                            }
                            result += expr_to_string(initializer.arguments.at(i).value);
                        }
                        result += ")";
                    }
                });
            result += " )";
            return result;
        }

        std::string operator()(expression_delete const& expr) const
        {
            return "( DELETE " + expr_to_string(expr.pointer) + " )";
        }

        std::string operator()(expression_sizeof const& bits) const
        {
            return "SIZEOF(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_alignof const& align) const
        {
            return "ALIGNOF(" + to_string(align.of_type) + ")";
        }

        std::string operator()(expression_is_signed const& bits) const
        {
            return "IS_SIGNED(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_is_integral const& bits) const
        {
            return "IS_INTEGRAL(" + to_string(bits.of_type) + ")";
        }

        std::string operator()(expression_type_is_layoutless const& expression) const
        {
            return "TYPE_IS_LAYOUTLESS(" + to_string(expression.of_type) + ")";
        }

        std::string operator()(expression_same_types const& types) const
        {
            return "SAME_TYPES(" + to_string(types.lhs_type) + ", " + to_string(types.rhs_type) + ")";
        }

        std::string operator()(expression_type_index_of const& expression) const
        {
            return "TYPE_INDEX_OF(" + to_string(expression.indexed_type) + ")";
        }

        std::string operator()(expression_numeric_literal_fits const& fits) const
        {
            return "__NUMERIC_LITERAL_FITS(" + to_string(fits.literal_type) + ", " + to_string(fits.target_type) + ")";
        }

        std::string operator()(expression_numeric_literal_binary_op const& op) const
        {
            return "__NUMERIC_LITERAL_" + op.op + "(" + to_string(op.lhs_type) + ", " + to_string(op.rhs_type) + ")";
        }

        std::string operator()(expression_numeric_literal_negate const& neg) const
        {
            return "__NUMERIC_LITERAL_NEGATE(" + to_string(neg.operand_type) + ")";
        }

        std::string operator()(expression_unary_postfix const& be) const
        {
            return "(" + expr_to_string(be.lhs) + " " + be.operator_str + ")";
        }

        std::string operator()(expression_unary_prefix const& be) const
        {
            return "(" + be.operator_str + " " + expr_to_string(be.rhs) + ")";
        }

        std::string operator()(expression_value_keyword const& kw) const
        {
            return kw.keyword;
        }

        std::string operator()(expression_have_cpu_attribute const& expression) const
        {
            return "HAVE_" + format_cpu_attribute_stem(expression.cpu_type, expression.attribute);
        }

        std::string operator()(expression_static_choose const& expr) const
        {
            return "STATIC_CHOOSE( " + expr_to_string(expr.condition) + " , " + expr_to_string(expr.true_expr) + " , " + expr_to_string(expr.false_expr) + " )";
        }

        std::string operator()(expression_snapshot const& expr) const
        {
            return "SNAPSHOT(" + expr.name + ")";
        }

        /** Formats a structural literal without discarding evaluation order. */
        std::string operator()(expression_composite_literal const& expr) const
        {
            std::string result = ":{";
            for (composite_field_initializer const& field : expr.fields)
            {
                result += " ." + field.name + " = " + expr_to_string(field.value) + ";";
            }
            return result + " }";
        }

        /** Formats public field metadata for diagnostics and expression identity. */
        std::string operator()(expression_public_field_metadata const& expr) const
        {
            std::array< std::string_view, 3 > names = {"PUBLIC_FIELD_COUNT", "PUBLIC_FIELD_NAME", "PUBLIC_FIELD_CONTAINS"};
            std::string result = std::string(names.at(static_cast< std::size_t >(expr.operation))) + "(" + to_string(expr.subject_type);
            if (expr.selector.has_value())
            {
                result += ", " + expr_to_string(*expr.selector);
            }
            return result + ")";
        }

        /** Formats a public field projection and its selector. */
        std::string operator()(expression_public_field_get const& expr) const
        {
            return "PUBLIC_FIELD_GET(" + expr_to_string(expr.source) + ", " + expr_to_string(expr.selector) + ")";
        }

        /** Formats the operation and all operands for diagnostics and expression identity. */
        std::string operator()(expression_composite_operation const& expr) const
        {
            static std::array< std::string_view, 10 > const names = {"COMPOSITE_CONTAINS", "COMPOSITE_FIELD_COUNT", "COMPOSITE_FIELD_NAME", "COMPOSITE_FIELD_GET", "COMPOSITE_TIE", "COMPOSITE_FORWARD", "COMPOSITE_JOIN", "COMPOSITE_SELECT", "COMPOSITE_EXCLUDE", "COMPOSITE_SPLIT"};
            std::string result = std::string(names.at(static_cast< std::size_t >(expr.operation))) + "(";
            bool first = true;
            if (expr.subject_type.has_value())
            {
                result += to_string(*expr.subject_type);
                first = false;
            }
            for (expression const& operand : expr.operands)
            {
                if (!first)
                {
                    result += ", ";
                }
                result += expr_to_string(operand);
                first = false;
            }
            return result + ")";
        }

        std::string operator()(expression_pack_size const& expr) const
        {
            return "PACK_SIZE(" + expr.pack_name + ")";
        }

        std::string operator()(expression_pack_arg const& expr) const
        {
            return "PACK_ARG(" + expr.pack_name + ", " + expr_to_string(expr.index) + ")";
        }

        std::string operator()(expression_forward const& expr) const
        {
            return "FORWARD(" + to_string(expr.symbol) + ")";
        }

        std::string operator()(expression_move const& expr) const
        {
            return "MOVE(" + to_string(expr.symbol) + ")";
        }

        std::string operator()(expression_choose const& expr) const
        {
            return "CHOOSE( " + expr_to_string(expr.condition) + " , " + expr_to_string(expr.true_expr) + " , " + expr_to_string(expr.false_expr) + " )";
        }

        std::string operator()(expression_begin_alloc_region const& expr) const
        {
            return "( BEGIN_ALLOC_REGION " + expr_to_string(expr.address) + " TO " + to_string(expr.as_type) + " )";
        }

        std::string operator()(expression_address_launder_discover_existing const& expr) const
        {
            return "( ADDRESS_LAUNDER_DISCOVER_EXISTING " + expr_to_string(expr.address) + " TO " + to_string(expr.to_type) + " )";
        }

        std::string operator()(expression_address_launder_escape_alloc_region const& expr) const
        {
            return "( ADDRESS_LAUNDER_ESCAPE_ALLOC_REGION " + expr_to_string(expr.pointer) + " )";
        }

        std::string operator()(expression_end_alloc_region const& expr) const
        {
            return "( END_ALLOC_REGION " + expr_to_string(expr.pointer) + " )";
        }

        std::string operator()(expression_begin_multi_alloc_region const& expr) const
        {
            return "( BEGIN_MULTI_ALLOC_REGION " + expr_to_string(expr.address) + " SIZE " + expr_to_string(expr.count) + " TO " + to_string(expr.as_type) + " )";
        }

        std::string operator()(expression_end_multi_alloc_region const& expr) const
        {
            std::string result = "( END_MULTI_ALLOC_REGION " + expr_to_string(expr.pointer);
            if (expr.count.has_value())
            {
                result += " SIZE " + expr_to_string(*expr.count);
            }
            result += " )";
            return result;
        }

        std::string operator()(expression_resize_multi_alloc_region const& expr) const
        {
            return "( RESIZE_MULTI_ALLOC_REGION " + expr_to_string(expr.pointer) + " COUNT " + expr_to_string(expr.newcount) + " )";
        }

        std::string operator()(expression_begin_dynamic_alloc_region const& expr) const
        {
            return "( BEGIN_DYNAMIC_ALLOC_REGION " + expr_to_string(expr.address) + " SIZE " + expr_to_string(expr.count) + " )";
        }

        std::string operator()(expression_end_dynamic_alloc_region const& expr) const
        {
            return "( END_DYNAMIC_ALLOC_REGION " + expr_to_string(expr.address) + " SIZE " + expr_to_string(expr.count) + " )";
        }

        std::string operator()(expression_resize_dynamic_alloc_region const& expr) const
        {
            return "( RESIZE_DYNAMIC_ALLOC_REGION " + expr_to_string(expr.address) + " SIZE " + expr_to_string(expr.newsize) + " )";
        }

        std::string operator()(expression_parent_alloc_address const& expr) const
        {
            return "( PARENT_ALLOC_ADDRESS " + expr_to_string(expr.pointer_or_address) + " )";
        }

        std::string operator()(expression_relocate_region_objects const& expr) const
        {
            return "( RELOCATE_REGION_OBJECTS FROM " + expr_to_string(expr.from) + " TO " + expr_to_string(expr.to) + " SIZE " + expr_to_string(expr.byte_count) + " )";
        }

        std::string operator()(expression_lambda const& expr) const
        {
            std::string result = "-<";
            if (expr.has_explicit_capture_list)
            {
                result += " [";
                for (std::size_t i = 0; i < expr.captures.size(); i++)
                {
                    if (i != 0)
                    {
                        result += ", ";
                    }
                    if (expr.captures.at(i).mode == lambda_capture_mode::value)
                    {
                        result += "=";
                    }
                    result += expr.captures.at(i).name;
                }
                result += "]";
            }
            result += " <lambda>";
            return result;
        }

        std::string operator()(expression_multibind const& brkts) const
        {
            std::string result;
            result += expr_to_string(brkts.lhs);
            result += " [ ";
            for (std::size_t i = 0; i < brkts.bracketed.size(); i++)
            {
                result += expr_to_string(brkts.bracketed[i]);
                if (i != brkts.bracketed.size() - 1)
                {
                    result += " , ";
                }
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
            return "(" + expr_to_string(expr.lhs) + "." + expr.field_name + ")";
        }

        std::string operator()(expression_call const& expr) const
        {
            if (expr.composite_arguments.has_value())
            {
                return "(APPLY " + expr_to_string(*expr.composite_arguments) + " TO " + expr_to_string(expr.callee) + ")";
            }
            std::string result = "(" + to_string(expr.callee) + "(";
            for (decltype(expr.args)::size_type i = 0; i < expr.args.size(); i++)
            {
                auto arg = expr.args[i].value;
                result += expr_to_string(arg);
                if (i != expr.args.size() - 1)
                {
                    result += ", ";
                }
            }
            result += "))";
            return result;
        }

        std::string operator()(expression_multiply const& expr) const
        {
            return "(" + expr_to_string(expr.lhs) + " * " + expr_to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_binary const& expr) const
        {
            return "(" + expr_to_string(expr.lhs) + " " + expr.operator_str + " " + expr_to_string(expr.rhs) + ")";
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
            {
                return "'\\n'";
            }
            else if (val == '\t')
            {
                return "'\\t'";
            }
            else if (val == '\r')
            {
                return "'\\r'";
            }
            else if (val == '\'')
            {
                return "'\\''";
            }
            else if (val == '\\')
            {
                return "'\\\\'";
            }
            else if (val == '\0')
            {
                return "'\\0'";
            }
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
            std::string output = "(" + expr_to_string(expr.lhs) + " <- )";
            return output;
        }

        std::string operator()(expression_rightarrow const& expr) const
        {
            std::string output = "(" + expr_to_string(expr.lhs) + " -> )";
            return output;
        }
    };

    struct type_symbol_stringifier
    {
        std::string operator()(storage const& ref) const;
        std::string operator()(aligned_storage const& ref) const;
        std::string operator()(virtual_storage const& ref) const;
        std::string operator()(readonly_constant const& ref) const;
        std::string operator()(freebound_identifier const& ref) const;
        std::string operator()(builtin_symbol const& ref) const;
        std::string operator()(context_reference const& ref) const;
        std::string operator()(subsymbol const& ref) const;
        std::string operator()(subtag_type const& ref) const;
        std::string operator()(initialization_reference const& ref) const;
        std::string operator()(absolute_module_reference const& ref) const;
        std::string operator()(attached_type_reference const& ref) const;
        std::string operator()(int_type const& ref) const;
        std::string operator()(float_type const& ref) const;

        std::string operator()(byte_type const& ref) const;
        std::string operator()(initguard_type const& ref) const;
        std::string operator()(initguard_lock_type const& ref) const;
        std::string operator()(constexpr_proxy const& ref) const;
        std::string operator()(bool_type const& ref) const;
        std::string operator()(array_type const& arr) const;
        std::string operator()(size_type const& ref) const;
        std::string operator()(address_type const& ref) const;
        std::string operator()(type_index_type const& ref) const;
        std::string operator()(value_expression_reference const& ref) const;
        std::string operator()(submember const& ref) const;
        std::string operator()(void_type const&) const;
        std::string operator()(null_type const&) const;
        std::string operator()(thistype const&) const;
        std::string operator()(numeric_literal_type const&) const;
        std::string operator()(numeric_literal_any_temploidic const&) const;
        std::string operator()(auto_temploidic const&) const;
        std::string operator()(decay_temploidic const&) const;
        std::string operator()(type_temploidic const&) const;
        std::string operator()(temploid_reference const&) const;
        std::string operator()(function_arg const&) const;
        std::string operator()(nvalue_slot const&) const;
        std::string operator()(dvalue_slot const&) const;
        std::string operator()(procedure_type const&) const;
        std::string operator()(ptrref_type const&) const;
        std::string operator()(instanciation_reference const&) const;
        std::string operator()(string_literal_type const&) const;
        std::string operator()(string_literal_any_temploidic const&) const;
        std::string operator()(array_initializer_type const&) const;
        /// Formats an internal function-local static storage symbol.
        std::string operator()(static_local_ref const&) const;
        /// Formats an internal function-local static snapshot symbol.
        std::string operator()(static_snapshot_ref const&) const;
        /** Formats the canonical field schema of an anonymous struct. */
        std::string operator()(composite_type const& type) const
        {
            std::string result = "COMPOSITE{";
            for (std::pair< std::string const, type_symbol > const& field : type.fields)
            {
                result += "." + field.first + " " + to_string(field.second) + ";";
            }
            return result + "}";
        }
        /** Formats a deferred public field type expression. */
        std::string operator()(public_field_type_ref const& type) const
        {
            return "PUBLIC_FIELD_TYPE(" + to_string(type.subject_type) + ", " + to_string(type.selector) + ")";
        }
        /** Formats a deferred field type expression. */
        std::string operator()(composite_field_type_ref const& type) const
        {
            return "COMPOSITE_FIELD_TYPE(" + to_string(type.composite) + ", " + to_string(type.selector) + ")";
        }
        /// Formats a pack argument type query.
        std::string operator()(pack_arg_type_ref const&) const;
        std::string operator()(decltype_type_ref const&) const;
        std::string operator()(typeof_type_ref const&) const;

      public:
        type_symbol_stringifier() = default;
    };

    struct named_argument_printer
    {
        template < typename NamedMap, typename PrintEntry >
        static void append(std::string& output, bool& first, NamedMap const& named, PrintEntry print_entry)
        {
            auto append_entry = [&](auto const& entry)
            {
                if (!first)
                {
                    output += ", ";
                }
                first = false;
                print_entry(entry);
            };

            if (auto this_arg = named.find("THIS"); this_arg != named.end())
            {
                append_entry(*this_arg);
            }
            if (auto other_arg = named.find("OTHER"); other_arg != named.end())
            {
                append_entry(*other_arg);
            }

            for (auto const& entry : named)
            {
                if (entry.first == "THIS" || entry.first == "OTHER")
                {
                    continue;
                }
                append_entry(entry);
            }
        }
    };

    std::string to_string(vmir2::invocation_args const& ref)
    {
        std::string result = "[";
        bool first = true;
        named_argument_printer::append(result, first, ref.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, arg] = entry;
            result += "@" + name + " " + std::to_string(arg);
        });
        for (auto const& arg : ref.positional)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += ", ";
            }
            result += std::to_string(arg);
        }
        result += "]";
        return result;
    }
    std::string to_string(codegen_invocation_args const& ref)
    {
        std::string result = "[";
        bool first = true;
        named_argument_printer::append(result, first, ref.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, arg] = entry;
            result += "@" + name + " " + std::to_string(arg);
        });
        for (auto const& arg : ref.positional)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += ", ";
            }
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

        bool operator()(virtual_storage const&) const
        {
            return false;
        }

        bool operator()(array_initializer_type const& ref) const
        {
            return false;
        }

        /// Returns true when the static local owner contains template parameters.
        bool operator()(static_local_ref const& ref) const
        {
            return is_template(ref.functanoid);
        }

        /// Returns true when the snapshot owner contains template parameters.
        bool operator()(static_snapshot_ref const& ref) const
        {
            return is_template(ref.functanoid);
        }

        /** Tests structural fields for template type patterns. */
        bool operator()(composite_type const& type) const
        {
            for (std::pair< std::string const, type_symbol > const& field : type.fields)
            {
                if (is_template(field.second))
                {
                    return true;
                }
            }
            return false;
        }
        /** Public field lookup is resolved in its surrounding context. */
        bool operator()(public_field_type_ref const&) const
        {
            return false;
        }

        /** Field lookup is resolved in its surrounding context. */
        bool operator()(composite_field_type_ref const&) const
        {
            return false;
        }

        bool operator()(pack_arg_type_ref const& ref) const
        {
            return false;
        }

        bool operator()(decltype_type_ref const& ref) const
        {
            return is_template(ref.symbol);
        }

        bool operator()(typeof_type_ref const&) const
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

        bool operator()(constexpr_proxy const&) const
        {
            return false;
        }

        bool operator()(string_literal_type const&) const
        {
            return false;
        }

        bool operator()(string_literal_any_temploidic const&) const
        {
            return true;
        }

        bool operator()(subsymbol const& ref) const
        {
            return is_template(ref.of);
        }

        bool operator()(subtag_type const& ref) const
        {
            return is_template(ref.of);
        }

        bool operator()(size_type const&)
        {
            return false;
        }

        bool operator()(address_type const&)
        {
            return false;
        }

        bool operator()(type_index_type const&)
        {
            return false;
        }

        bool operator()(freebound_identifier const& ref) const
        {
            return false; // Freebound identifiers are not templates
        }

        bool operator()(builtin_symbol const&) const
        {
            return false;
        }

        bool operator()(instanciation_reference const& type)
        {
            for (auto& p : type.params.positional)
            {
                if (is_template(parameter_instantiation_type(p)))
                {
                    return true;
                }
            }
            for (auto& p : type.params.named)
            {
                if (is_template(parameter_instantiation_type(p.second)))
                {
                    return true;
                }
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
            {
                return true;
            }
            for (auto& p : ref.parameters.positional)
            {
                if (is_template(parameter_instantiation_type(p)))
                {
                    return true;
                }
            }
            for (auto const& [_, param] : ref.parameters.named)
            {
                if (is_template(parameter_instantiation_type(param)))
                {
                    return true;
                }
            }
            return false;
        }

        bool operator()(temploid_reference const& ref) const
        {
            if (is_template(ref.templexoid))
            {
                return true;
            }
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

        bool operator()(float_type const& ref) const
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

        bool operator()(null_type const&) const
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

        bool operator()(numeric_literal_type const&) const
        {
            return false;
        }

        bool operator()(auto_temploidic const&) const
        {
            return true;
        }

        bool operator()(decay_temploidic const&) const
        {
            return true;
        }

        bool operator()(type_temploidic const&) const
        {
            return true;
        }

        bool operator()(numeric_literal_any_temploidic const&) const
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
        else if (input.template type_is< subtag_type >())
        {
            return as< subtag_type >(input).of;
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
        else if (input.template type_is< static_local_ref >())
        {
            return as< static_local_ref >(input).functanoid;
        }
        else if (input.template type_is< static_snapshot_ref >())
        {
            return as< static_snapshot_ref >(input).functanoid;
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
        else if (ref.template type_is< subtag_type >())
        {
            return subtag_type{with_context(as< subtag_type >(ref).of, context), as< subtag_type >(ref).name};
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
            if (!output.context.has_value())
            {
                output.context = context;
            }
            for (auto& p : output.parameters.positional)
            {
                if (p.template type_is< parameter_type_instantiation >())
                {
                    p.template get_as< parameter_type_instantiation >().type = with_context(p.template get_as< parameter_type_instantiation >().type, context);
                }
                else
                {
                    p.template get_as< parameter_value_instantiation >().type = with_context(p.template get_as< parameter_value_instantiation >().type, context);
                }
            }
            for (auto& [_, p] : output.parameters.named)
            {
                if (p.template type_is< parameter_type_instantiation >())
                {
                    p.template get_as< parameter_type_instantiation >().type = with_context(p.template get_as< parameter_type_instantiation >().type, context);
                }
                else
                {
                    p.template get_as< parameter_value_instantiation >().type = with_context(p.template get_as< parameter_value_instantiation >().type, context);
                }
            }
            return output;
        }
        else if (ref.template type_is< static_local_ref >())
        {
            static_local_ref output = as< static_local_ref >(ref);
            output.functanoid = with_context(output.functanoid, context);
            return output;
        }
        else if (ref.template type_is< static_snapshot_ref >())
        {
            static_snapshot_ref output = as< static_snapshot_ref >(ref);
            output.functanoid = with_context(output.functanoid, context);
            return output;
        }
        else if (ref.template type_is< pack_arg_type_ref >())
        {
            return ref;
        }
        else if (ref.template type_is< decltype_type_ref >())
        {
            decltype_type_ref output = as< decltype_type_ref >(ref);
            output.symbol = with_context(output.symbol, context);
            return output;
        }
        else if (ref.template type_is< typeof_type_ref >())
        {
            return ref;
        }
        else
        {
            return ref;
        }
    }

    bool type_symbol_needs_postfix_receiver_parentheses(type_symbol const& ref)
    {
        return typeis< ptrref_type >(ref) || typeis< array_type >(ref) || typeis< nvalue_slot >(ref) || typeis< dvalue_slot >(ref);
    }

    std::string to_string_as_postfix_receiver(type_symbol const& ref)
    {
        auto output = to_string(ref);
        if (type_symbol_needs_postfix_receiver_parentheses(ref))
        {
            return "(" + output + ")";
        }
        return output;
    }

    std::string type_symbol_stringifier::operator()(subsymbol const& ref) const
    {
        return to_string_as_postfix_receiver(ref.of) + "::" + ref.name;
    }

    std::string type_symbol_stringifier::operator()(subtag_type const& ref) const
    {
        return to_string_as_postfix_receiver(ref.of) + "$" + ref.name;
    }

    std::string type_symbol_stringifier::operator()(size_type const& ref) const
    {
        return "SZ";
    }

    std::string type_symbol_stringifier::operator()(address_type const& ref) const
    {
        return "ADDRESS";
    }

    std::string type_symbol_stringifier::operator()(type_index_type const& ref) const
    {
        return "TYPE_INDEX";
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
            break;
        case pointer_class::gc:
            output += "~>";
            break;
        }

        output += " ";
        output += to_string(ref.target);

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
        named_argument_printer::append(output, first, ref.signature.params.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, arg] = entry;
            output += "@" + name + " " + to_string(arg);
        });
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
        std::string output = to_string_as_postfix_receiver(ref.initializee);
        output += " #(";
        bool first = true;
        if (!ref.arguments.empty())
        {
            for (std::size_t argument_index = 0; argument_index < ref.arguments.size();)
            {
                if (!first)
                {
                    output += ", ";
                }
                first = false;
                expression_arg const& arg = ref.arguments.at(argument_index);
                if (arg.name.has_value())
                {
                    output += "@" + *arg.name + " ";
                    output += to_string(arg.value);
                    argument_index++;
                    continue;
                }

                output += "% [";
                bool first_positional = true;
                while (argument_index < ref.arguments.size() && !ref.arguments.at(argument_index).name.has_value())
                {
                    if (!first_positional)
                    {
                        output += ", ";
                    }
                    first_positional = false;
                    output += to_string(ref.arguments.at(argument_index).value);
                    argument_index++;
                }
                output += "]";
            }
        }
        else
        {
            named_argument_printer::append(output, first, ref.parameters.named,
                                           [&](auto const& entry)
                                           {
                auto const& [name, param] = entry;
                output += "@" + name + " " + to_string(param);
            });
            if (!ref.parameters.positional.empty())
            {
                if (!first)
                {
                    output += ", ";
                }
                first = false;
                output += "% [";
                bool first_positional = true;
                for (parameter_instantiation const& param : ref.parameters.positional)
                {
                    if (!first_positional)
                    {
                        output += ", ";
                    }
                    first_positional = false;
                    output += to_string(param);
                }
                output += "]";
            }
        }
        output += ")";
        return output;
    }

    std::string type_symbol_stringifier::operator()(instanciation_reference const& ref) const
    {
        temploid_reference const& sel = ref.temploid;
        std::string output = to_string_as_postfix_receiver(sel.templexoid);
        if (sel.overload_id.has_value())
        {
            output += "!$[" + std::to_string(*sel.overload_id) + "]{";
        }
        else
        {
            output += "#{";
        }

        bool first = true;
        named_argument_printer::append(output, first, ref.params.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, actual] = entry;
            output += "@" + name + " " + to_string(actual);
        });
        for (auto const& actual : ref.params.positional)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += to_string(actual);
        }
        output += "}";
        return output;
    }

    std::string type_symbol_stringifier::operator()(storage const& ref) const
    {
        std::string result = "TYPED_STORAGE(";
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
    std::string type_symbol_stringifier::operator()(virtual_storage const&) const
    {
        return "VIRTUAL_STORAGE";
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
    std::string type_symbol_stringifier::operator()(builtin_symbol const& ref) const
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
    std::string type_symbol_stringifier::operator()(float_type const& ref) const
    {
        if (ref.bits == 32 && ref.exponent_bits == 8)
        {
            return "F32";
        }
        if (ref.bits == 64 && ref.exponent_bits == 11)
        {
            return "F64";
        }
        return "F" + std::to_string(ref.bits) + "E" + std::to_string(ref.exponent_bits);
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
    std::string type_symbol_stringifier::operator()(constexpr_proxy const& ref) const
    {
        return "__CONSTEXPR_PROXY";
    }
    std::string type_symbol_stringifier::operator()(value_expression_reference const& ref) const
    {
        return "VALUE";
    }
    std::string type_symbol_stringifier::operator()(void_type const&) const
    {
        return "VOID";
    }

    std::string type_symbol_stringifier::operator()(null_type const&) const
    {
        return "NULL_TYPE";
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
        return to_string_as_postfix_receiver(ref.of) + "::." + ref.name;
    }

    std::string type_symbol_stringifier::operator()(numeric_literal_type const& ref) const
    {
        return "NUMERIC_LITERAL_TYPE(\"" + ref.value + "\")";
    }
    std::string type_symbol_stringifier::operator()(numeric_literal_any_temploidic const& ref) const
    {
        return "NUMERIC_LITERAL_ANY(" + ref.name + ")";
    }

    std::string type_symbol_stringifier::operator()(string_literal_type const& ref) const
    {
        return "STRING_LITERAL_TYPE(\"" + ref.value + "\")";
    }

    std::string type_symbol_stringifier::operator()(string_literal_any_temploidic const& ref) const
    {
        return "STRING_LITERAL_ANY(" + ref.name + ")";
    }

    std::string type_symbol_stringifier::operator()(array_initializer_type const& ai) const
    {
        return "__ARRAY_INITIALIZER( " + to_string(ai.element_type) + ", " + std::to_string(ai.count) + " )";
    }

    /// Formats an internal function-local static storage symbol.
    std::string type_symbol_stringifier::operator()(static_local_ref const& ref) const
    {
        return to_string_as_postfix_receiver(ref.functanoid) + "::__STATIC_LOCAL(" + ref.name + ", " + std::to_string(ref.generation) + ")";
    }

    /// Formats an internal function-local static snapshot symbol.
    std::string type_symbol_stringifier::operator()(static_snapshot_ref const& ref) const
    {
        return to_string_as_postfix_receiver(ref.functanoid) + "::__STATIC_SNAPSHOT(" + ref.name + ", " + std::to_string(ref.generation) + ", " + std::to_string(ref.snapshot_id) + ")";
    }

    std::string type_symbol_stringifier::operator()(pack_arg_type_ref const& ref) const
    {
        return "PACK_ARG_TYPE(" + ref.pack_name + ", " + to_string(ref.index) + ")";
    }
    std::string type_symbol_stringifier::operator()(decltype_type_ref const& ref) const
    {
        return "DECLTYPE(" + to_string(ref.symbol) + ")";
    }
    std::string type_symbol_stringifier::operator()(typeof_type_ref const& ref) const
    {
        return "TYPEOF(" + to_string(ref.expr) + ")";
    }

    std::string type_symbol_stringifier::operator()(auto_temploidic const& val) const
    {
        return "AUTO(" + val.name + ")";
    }
    std::string type_symbol_stringifier::operator()(decay_temploidic const& val) const
    {
        if (val.name.empty())
        {
            return "DECAY";
        }
        return "DECAY(" + val.name + ")";
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
        std::string output = to_string_as_postfix_receiver(ref.templexoid) + "!$[";
        if (ref.overload_id.has_value())
        {
            output += std::to_string(*ref.overload_id);
        }
        output += "]";
        return output;
    }

    std::string to_string(argif const& arg)
    {
        std::string output;
        if (arg.is_pack)
        {
            output += "...";
        }
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
        named_argument_printer::append(output, first, ref.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, arg] = entry;
            output += "@" + name + " " + to_string(arg);
        });
        for (auto const& arg : ref.positional)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += to_string(arg);
        }
        output += ")";
        return output;
    }

    std::string to_string(interface_slot_key const& ref)
    {
        std::string output = "@" + ref.name;
        if (!ref.concrete_params.named.empty() || !ref.concrete_params.positional.empty() || ref.concrete_return_type.has_value())
        {
            output += "#(";
            bool first = true;
            named_argument_printer::append(output, first, ref.concrete_params.named,
                                           [&](std::pair< std::string const, type_symbol > const& entry)
                                           {
                std::string const& name = entry.first;
                type_symbol const& arg = entry.second;
                output += "@" + name + " " + to_string(arg);
            });
            if (!ref.concrete_params.positional.empty())
            {
                if (!first)
                {
                    output += ", ";
                }
                first = false;
                output += "% [";
                bool first_positional = true;
                for (type_symbol const& arg : ref.concrete_params.positional)
                {
                    if (!first_positional)
                    {
                        output += ", ";
                    }
                    first_positional = false;
                    output += to_string(arg);
                }
                output += "]";
            }
            if (ref.concrete_return_type.has_value())
            {
                if (!first)
                {
                    output += ": ";
                }
                else
                {
                    output += ": ";
                }
                output += to_string(*ref.concrete_return_type);
            }
            output += ")";
        }
        return output;
    }

    std::string constexpr_parameter_value_to_string(type_symbol const& type, constexpr_value const& value)
    {
        auto const& antestatal = constexpr_value_as_antestatal(value);
        if (typeis< bool_type >(type) && typeis< antestatal_primitive >(antestatal))
        {
            auto const& data = as< antestatal_primitive >(antestatal).value;
            if (data == std::vector{std::byte{0}})
            {
                return "FALSE";
            }
            if (data == std::vector{std::byte{1}})
            {
                return "TRUE";
            }
        }
        if ((typeis< int_type >(type) || typeis< byte_type >(type)) && typeis< antestatal_primitive >(antestatal))
        {
            auto [value_u64, ok] = bytemath::le_to_u< std::uint64_t >(as< antestatal_primitive >(antestatal).value);
            if (ok)
            {
                return std::to_string(value_u64);
            }
        }
        return "<constexpr>";
    }

    std::string to_string(parameter_instantiation const& ref)
    {
        if (ref.template type_is< parameter_type_instantiation >())
        {
            return to_string(ref.template get_as< parameter_type_instantiation >().type);
        }

        auto const& value = ref.template get_as< parameter_value_instantiation >();
        return to_string(value.type) + "=" + constexpr_parameter_value_to_string(value.type, value.value);
    }

    std::string to_string(instatype const& ref)
    {
        std::string output;
        bool first = true;
        output += "INSTATYPE(";
        named_argument_printer::append(output, first, ref.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, arg] = entry;
            output += "@" + name + " " + to_string(arg);
        });
        for (auto const& arg : ref.positional)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
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
        named_argument_printer::append(output, first, ref.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, arg] = entry;
            output += "@" + name + " " + to_string(arg);
        });
        for (auto const& arg : ref.positional)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += to_string(arg);
        }
        if (ref.accepts_named_rest)
        {
            if (!first)
            {
                output += ", ";
            }
            output += "@KWARGS ...";
        }
        output += ")";
        return output;
    }

    std::string to_string(vmir2::routine_parameters const& ref)
    {
        std::string output;
        bool first = true;
        output += "PARAMETERS(";
        named_argument_printer::append(output, first, ref.named,
                                       [&](auto const& entry)
                                       {
            auto const& [name, arg] = entry;
            output += "@" + name + " " + to_string(arg.type) + "=%" + std::to_string(arg.local_index);
        });
        for (auto const& arg : ref.positional)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += to_string(arg.type) + "=%" + std::to_string(arg.local_index);
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
        type_symbol tmplx = func.get_as< instanciation_reference >().temploid.templexoid;
        if (tmplx.type_is< submember >())
        {
            return as< submember >(tmplx).of;
        }
        if (tmplx.type_is< instanciation_reference >())
        {
            type_symbol const& instantiated_temploid = as< instanciation_reference >(tmplx).temploid.templexoid;
            if (instantiated_temploid.type_is< submember >())
            {
                return as< submember >(instantiated_temploid).of;
            }
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
        case pointer_class::gc:
            if (match_class == pointer_class::gc)
            {
                return pointer_class::gc;
            }
            return std::nullopt;
        }

        throw compiler_bug("should be unreachable");
    }


} // namespace quxlang


std::string quxlang::to_string(quxlang::type_symbol const& ref)
{
    return rpnx::apply_visitor< std::string >(ref, quxlang::type_symbol_stringifier{});
}

std::string quxlang::to_string(expression const& expr)
{
    return to_string(expr, false);
}

std::string quxlang::to_string(expression const& expr, bool print_locations)
{
    auto str = rpnx::apply_visitor< std::string, rpnx::dispatch_type::branching >(expr, expression_stringifier{print_locations});
    if (print_locations)
    {
        str += source_location_suffix(get_location(expr));
    }

    // TODO: replace all "  " with " "
    return str;
}

quxlang::expression quxlang::strip_source_locations(expression expr)
{
    rpnx::apply_visitor< void >(expr,
        [](auto& value)
        {
            value.location = std::nullopt;
            using value_type = std::decay_t< decltype(value) >;
            if constexpr (std::is_same_v< value_type, expression_binary >)
            {
                value.lhs = strip_source_locations(std::move(value.lhs));
                value.rhs = strip_source_locations(std::move(value.rhs));
            }
            else if constexpr (std::is_same_v< value_type, expression_unary_prefix >)
            {
                value.rhs = strip_source_locations(std::move(value.rhs));
            }
                                    else if constexpr (std::is_same_v< value_type, expression_unary_postfix > || std::is_same_v< value_type, expression_dotreference > || std::is_same_v< value_type, expression_rightarrow > || std::is_same_v< value_type, expression_leftarrow >)
            {
                value.lhs = strip_source_locations(std::move(value.lhs));
            }
            else if constexpr (std::is_same_v< value_type, expression_multibind >)
            {
                value.lhs = strip_source_locations(std::move(value.lhs));
                for (auto& item : value.bracketed)
                {
                    item = strip_source_locations(std::move(item));
                }
            }
            else if constexpr (std::is_same_v< value_type, expression_call >)
            {
                value.callee = strip_source_locations(std::move(value.callee));
                if (value.composite_arguments.has_value())
                {
                    value.composite_arguments = strip_source_locations(std::move(*value.composite_arguments));
                }
                for (auto& arg : value.args)
                {
                    arg.location = std::nullopt;
                    arg.value = strip_source_locations(std::move(arg.value));
                }
            }
            else if constexpr (std::is_same_v< value_type, expression_composite_literal >)
            {
                for (composite_field_initializer& field : value.fields)
                {
                    field.location.reset();
                    field.value = strip_source_locations(std::move(field.value));
                }
            }
            else if constexpr (std::is_same_v< value_type, expression_public_field_metadata >)
            {
                value.subject_type = strip_source_locations(std::move(value.subject_type));
                if (value.selector.has_value())
                {
                    value.selector = strip_source_locations(std::move(*value.selector));
                }
            }
            else if constexpr (std::is_same_v< value_type, expression_public_field_get >)
            {
                value.source = strip_source_locations(std::move(value.source));
                value.selector = strip_source_locations(std::move(value.selector));
            }
            else if constexpr (std::is_same_v< value_type, expression_composite_operation >)
            {
                if (value.subject_type.has_value())
                {
                    value.subject_type = strip_source_locations(std::move(*value.subject_type));
                }
                for (expression& operand : value.operands)
                {
                    operand = strip_source_locations(std::move(operand));
                }
            }
            else if constexpr (std::is_same_v< value_type, expression_typecast >)
            {
                value.expr = strip_source_locations(std::move(value.expr));
                value.to_type = strip_source_locations(std::move(value.to_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_pun >)
            {
                value.value = strip_source_locations(std::move(value.value));
                value.as_type = strip_source_locations(std::move(value.as_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_place >)
            {
                value.at = strip_source_locations(std::move(value.at));
                value.type = strip_source_locations(std::move(value.type));
                if (value.assign_init)
                {
                    value.assign_init = strip_source_locations(std::move(*value.assign_init));
                }
                for (auto& arg : value.args)
                {
                    arg.location = std::nullopt;
                    arg.value = strip_source_locations(std::move(arg.value));
                }
            }
            else if constexpr (std::is_same_v< value_type, expression_new >)
            {
                value.type = strip_source_locations(std::move(value.type));
                rpnx::apply_visitor< void >(value.initializer,
                    [](auto& initializer)
                    {
                        using initializer_type = std::decay_t< decltype(initializer) >;
                        if constexpr (std::is_same_v< initializer_type, new_from_initializer >)
                        {
                            initializer.source = strip_source_locations(std::move(initializer.source));
                        }
                        else if constexpr (std::is_same_v< initializer_type, new_arguments_initializer >)
                        {
                            for (expression_arg& argument : initializer.arguments)
                            {
                                argument.location = std::nullopt;
                                argument.value = strip_source_locations(std::move(argument.value));
                            }
                        }
                    });
            }
            else if constexpr (std::is_same_v< value_type, expression_delete >)
            {
                value.pointer = strip_source_locations(std::move(value.pointer));
            }
            else if constexpr (std::is_same_v< value_type, expression_choose > || std::is_same_v< value_type, expression_static_choose >)
            {
                value.condition = strip_source_locations(std::move(value.condition));
                value.true_expr = strip_source_locations(std::move(value.true_expr));
                value.false_expr = strip_source_locations(std::move(value.false_expr));
            }
            else if constexpr (std::is_same_v< value_type, expression_address_launder_discover_existing >)
            {
                value.address = strip_source_locations(std::move(value.address));
                value.to_type = strip_source_locations(std::move(value.to_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_address_launder_escape_alloc_region >)
            {
                value.pointer = strip_source_locations(std::move(value.pointer));
            }
            else if constexpr (std::is_same_v< value_type, expression_begin_alloc_region >)
            {
                value.address = strip_source_locations(std::move(value.address));
                value.as_type = strip_source_locations(std::move(value.as_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_end_alloc_region >)
            {
                value.pointer = strip_source_locations(std::move(value.pointer));
            }
            else if constexpr (std::is_same_v< value_type, expression_begin_multi_alloc_region >)
            {
                value.address = strip_source_locations(std::move(value.address));
                value.count = strip_source_locations(std::move(value.count));
                value.as_type = strip_source_locations(std::move(value.as_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_end_multi_alloc_region >)
            {
                value.pointer = strip_source_locations(std::move(value.pointer));
                if (value.count.has_value())
                {
                    value.count = strip_source_locations(std::move(*value.count));
                }
            }
            else if constexpr (std::is_same_v< value_type, expression_resize_multi_alloc_region >)
            {
                value.pointer = strip_source_locations(std::move(value.pointer));
                value.newcount = strip_source_locations(std::move(value.newcount));
            }
            else if constexpr (std::is_same_v< value_type, expression_begin_dynamic_alloc_region >)
            {
                value.address = strip_source_locations(std::move(value.address));
                value.count = strip_source_locations(std::move(value.count));
            }
            else if constexpr (std::is_same_v< value_type, expression_end_dynamic_alloc_region >)
            {
                value.address = strip_source_locations(std::move(value.address));
                value.count = strip_source_locations(std::move(value.count));
            }
            else if constexpr (std::is_same_v< value_type, expression_resize_dynamic_alloc_region >)
            {
                value.address = strip_source_locations(std::move(value.address));
                value.newsize = strip_source_locations(std::move(value.newsize));
            }
            else if constexpr (std::is_same_v< value_type, expression_parent_alloc_address >)
            {
                value.pointer_or_address = strip_source_locations(std::move(value.pointer_or_address));
            }
            else if constexpr (std::is_same_v< value_type, expression_relocate_region_objects >)
            {
                value.from = strip_source_locations(std::move(value.from));
                value.to = strip_source_locations(std::move(value.to));
                value.byte_count = strip_source_locations(std::move(value.byte_count));
            }
            else if constexpr (std::is_same_v< value_type, expression_symbol_reference >)
            {
                value.symbol = strip_source_locations(std::move(value.symbol));
            }
                                    else if constexpr (std::is_same_v< value_type, expression_bits > || std::is_same_v< value_type, expression_sizeof > || std::is_same_v< value_type, expression_alignof > || std::is_same_v< value_type, expression_is_integral > || std::is_same_v< value_type, expression_type_is_layoutless > || std::is_same_v< value_type, expression_is_signed >)
            {
                value.of_type = strip_source_locations(std::move(value.of_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_same_types >)
            {
                value.lhs_type = strip_source_locations(std::move(value.lhs_type));
                value.rhs_type = strip_source_locations(std::move(value.rhs_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_type_index_of >)
            {
                value.indexed_type = strip_source_locations(std::move(value.indexed_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_numeric_literal_fits >)
            {
                value.literal_type = strip_source_locations(std::move(value.literal_type));
                value.target_type = strip_source_locations(std::move(value.target_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_numeric_literal_binary_op >)
            {
                value.lhs_type = strip_source_locations(std::move(value.lhs_type));
                value.rhs_type = strip_source_locations(std::move(value.rhs_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_numeric_literal_negate >)
            {
                value.operand_type = strip_source_locations(std::move(value.operand_type));
            }
            else if constexpr (std::is_same_v< value_type, expression_pack_arg >)
            {
                value.index = strip_source_locations(std::move(value.index));
            }
            else if constexpr (std::is_same_v< value_type, expression_forward >)
            {
                value.symbol = strip_source_locations(std::move(value.symbol));
            }
            else if constexpr (std::is_same_v< value_type, expression_move >)
            {
                value.symbol = strip_source_locations(std::move(value.symbol));
            }
            else if constexpr (std::is_same_v< value_type, expression_lambda >)
            {
                for (auto& capture : value.captures)
                {
                    capture.location = std::nullopt;
                }
                for (auto& parameter : value.parameters)
                {
                    parameter.location = std::nullopt;
                    parameter.type = strip_source_locations(std::move(parameter.type));
                    if (parameter.default_expr.has_value())
                    {
                        parameter.default_expr = strip_source_locations(std::move(*parameter.default_expr));
                    }
                }
                if (value.return_type.has_value())
                {
                    value.return_type = strip_source_locations(std::move(*value.return_type));
                }
            }
        });
    return expr;
}

namespace quxlang::detail
{
    auto strip_invotype_locations(quxlang::invotype inv) -> quxlang::invotype
    {
        for (auto& item : inv.positional)
        {
            item = quxlang::strip_source_locations(std::move(item));
        }
        for (auto& [name, item] : inv.named)
        {
            (void)name;
            item = quxlang::strip_source_locations(std::move(item));
        }
        return inv;
    }

    auto strip_instatype_locations(quxlang::instatype inst) -> quxlang::instatype
    {
        auto strip_parameter = [](quxlang::parameter_instantiation param)
        {
            if (param.template type_is< quxlang::parameter_type_instantiation >())
            {
                auto& item = param.template get_as< quxlang::parameter_type_instantiation >();
                item.type = quxlang::strip_source_locations(std::move(item.type));
                return param;
            }
            auto& item = param.template get_as< quxlang::parameter_value_instantiation >();
            item.type = quxlang::strip_source_locations(std::move(item.type));
            return param;
        };

        for (auto& item : inst.positional)
        {
            item = strip_parameter(std::move(item));
        }
        for (auto& [name, item] : inst.named)
        {
            (void)name;
            item = strip_parameter(std::move(item));
        }
        return inst;
    }

    auto strip_argif_locations(quxlang::argif arg) -> quxlang::argif
    {
        arg.type = quxlang::strip_source_locations(std::move(arg.type));
        return arg;
    }

    auto strip_intertype_locations(quxlang::intertype inter) -> quxlang::intertype
    {
        for (auto& item : inter.positional)
        {
            item = strip_argif_locations(std::move(item));
        }
        for (auto& [name, item] : inter.named)
        {
            (void)name;
            item = strip_argif_locations(std::move(item));
        }
        return inter;
    }

    auto strip_sigtype_locations(quxlang::sigtype sig) -> quxlang::sigtype
    {
        sig.params = strip_invotype_locations(std::move(sig.params));
        if (sig.return_type)
        {
            sig.return_type = quxlang::strip_source_locations(std::move(*sig.return_type));
        }
        return sig;
    }
} // namespace quxlang::detail

quxlang::temploid_ensig quxlang::strip_source_locations(temploid_ensig ref)
{
    ref.interface = detail::strip_intertype_locations(std::move(ref.interface));
    if (ref.enable_if)
    {
        ref.enable_if = strip_source_locations(std::move(*ref.enable_if));
    }
    return ref;
}

quxlang::paratype quxlang::strip_source_locations(paratype ref)
{
    auto strip_parameter = [](parameter_type param)
    {
        param.type = strip_source_locations(std::move(param.type));
        if (param.default_value)
        {
            param.default_value = strip_source_locations(std::move(*param.default_value));
        }
        return param;
    };
    for (auto& item : ref.positional)
    {
        item = strip_parameter(std::move(item));
    }
    for (auto& [name, item] : ref.named)
    {
        (void)name;
        item = strip_parameter(std::move(item));
    }
    return ref;
}

quxlang::type_symbol quxlang::strip_source_locations(type_symbol ref)
{
    rpnx::apply_visitor< void >(ref,
        [](auto& value)
        {
            using value_type = std::decay_t< decltype(value) >;
            if constexpr (std::is_same_v< value_type, subsymbol > || std::is_same_v< value_type, submember > || std::is_same_v< value_type, subtag_type >)
            {
                value.of = strip_source_locations(std::move(value.of));
            }
            else if constexpr (std::is_same_v< value_type, procedure_type >)
            {
                value.signature = detail::strip_sigtype_locations(std::move(value.signature));
            }
            else if constexpr (std::is_same_v< value_type, ptrref_type > || std::is_same_v< value_type, nvalue_slot > || std::is_same_v< value_type, dvalue_slot >)
            {
                value.target = strip_source_locations(std::move(value.target));
            }
            else if constexpr (std::is_same_v< value_type, initialization_reference >)
            {
                value.initializee = strip_source_locations(std::move(value.initializee));
                if (value.context.has_value())
                {
                    value.context = strip_source_locations(std::move(*value.context));
                }
                for (auto& arg : value.arguments)
                {
                    arg.value = strip_source_locations(std::move(arg.value));
                }
                value.parameters = detail::strip_instatype_locations(std::move(value.parameters));
            }
            else if constexpr (std::is_same_v< value_type, temploid_reference >)
            {
                value.templexoid = strip_source_locations(std::move(value.templexoid));
            }
            else if constexpr (std::is_same_v< value_type, ensig_initialization >)
            {
                value.ensig = strip_source_locations(std::move(value.ensig));
                value.params = detail::strip_instatype_locations(std::move(value.params));
            }
            else if constexpr (std::is_same_v< value_type, instanciation_reference >)
            {
                value.temploid.templexoid = strip_source_locations(std::move(value.temploid.templexoid));
                value.params = detail::strip_instatype_locations(std::move(value.params));
            }
            else if constexpr (std::is_same_v< value_type, attached_type_reference >)
            {
                value.carrying_type = strip_source_locations(std::move(value.carrying_type));
                value.attached_symbol = strip_source_locations(std::move(value.attached_symbol));
            }
            else if constexpr (std::is_same_v< value_type, array_type >)
            {
                value.element_type = strip_source_locations(std::move(value.element_type));
                value.element_count = strip_source_locations(std::move(value.element_count));
            }
            else if constexpr (std::is_same_v< value_type, storage >)
            {
                std::set< type_symbol > stripped;
                for (auto item : value.storable_types)
                {
                    stripped.insert(strip_source_locations(std::move(item)));
                }
                value.storable_types = std::move(stripped);
            }
            else if constexpr (std::is_same_v< value_type, aligned_storage >)
            {
                value.size = strip_source_locations(std::move(value.size));
                value.align = strip_source_locations(std::move(value.align));
            }
            else if constexpr (std::is_same_v< value_type, static_local_ref >)
            {
                value.functanoid = strip_source_locations(std::move(value.functanoid));
            }
            else if constexpr (std::is_same_v< value_type, static_snapshot_ref >)
            {
                value.functanoid = strip_source_locations(std::move(value.functanoid));
            }
            else if constexpr (std::is_same_v< value_type, composite_type >)
            {
                for (std::pair< std::string const, type_symbol >& field : value.fields)
                {
                    field.second = strip_source_locations(std::move(field.second));
                }
            }
            else if constexpr (std::is_same_v< value_type, public_field_type_ref >)
            {
                value.subject_type = strip_source_locations(std::move(value.subject_type));
                value.selector = strip_source_locations(std::move(value.selector));
            }
            else if constexpr (std::is_same_v< value_type, composite_field_type_ref >)
            {
                value.composite = strip_source_locations(std::move(value.composite));
                value.selector = strip_source_locations(std::move(value.selector));
            }
            else if constexpr (std::is_same_v< value_type, pack_arg_type_ref >)
            {
                value.index = strip_source_locations(std::move(value.index));
            }
            else if constexpr (std::is_same_v< value_type, decltype_type_ref >)
            {
                value.symbol = strip_source_locations(std::move(value.symbol));
            }
            else if constexpr (std::is_same_v< value_type, typeof_type_ref >)
            {
                value.expr = strip_source_locations(std::move(value.expr));
            }
        });
    return ref;
}

bool quxlang::overload_has_unspecialized_parameters(temploid_ensig const& ensig)
{
    if (ensig.interface.accepts_named_rest)
    {
        return true;
    }
    for (argif const& param : ensig.interface.positional)
    {
        if (is_template(param.type))
        {
            return true;
        }
    }

    for (auto const& [name, param] : ensig.interface.named)
    {
        (void)name;
        if (is_template(param.type))
        {
            return true;
        }
    }

    return false;
}

quxlang::instatype quxlang::instantiate_declared_overload(temploid_ensig const& ensig)
{
    instatype result;
    for (argif const& param : ensig.interface.positional)
    {
        if (!param.is_pack)
        {
            result.positional.push_back(make_type_instantiation(param.type));
        }
    }

    for (auto const& [name, param] : ensig.interface.named)
    {
        result.named[name] = make_type_instantiation(param.type);
    }

    return result;
}
