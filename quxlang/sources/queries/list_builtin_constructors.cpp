// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/list_builtin_constructors_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::list_builtin_constructors_spec > quxlang::list_builtin_constructors_impl(type_symbol input)
{
    std::set< builtin_function_info > result;
    auto make_overload = [&](std::vector< type_symbol > positionals, std::map< std::string, type_symbol > named, type_symbol return_type, std::optional< expression > enable_if = std::nullopt, std::optional< std::int32_t > priority = std::nullopt)
    {
        builtin_function_info bl_info;
        for (auto& type : positionals)
        {
            bl_info.overload.interface.positional.push_back(argif{.type = type});
        }
        for (auto& [name, type] : named)
        {
            bl_info.overload.interface.named[name] = argif{.type = type};
        }
        bl_info.overload.enable_if = enable_if.has_value() ? std::move(enable_if) : std::optional< expression >{expression_value_keyword{.keyword = "TRUE"}};
        bl_info.overload.priority = priority.has_value() ? priority : std::optional< std::int32_t >{0};
        bl_info.return_type = return_type;
        return bl_info;
    };

    auto add_overload = [&](std::vector< type_symbol > positionals, std::map< std::string, type_symbol > named, type_symbol return_type, std::optional< expression > enable_if = std::nullopt, std::optional< std::int32_t > priority = std::nullopt)
    {
        result.insert(make_overload(positionals, named, return_type, std::move(enable_if), priority));
    };

    auto kw_expr = [](std::string keyword) -> expression
    {
        return expression_value_keyword{.keyword = std::move(keyword)};
    };

    auto false_expr = [&]() -> expression
    {
        return kw_expr("FALSE");
    };

    auto int_literal_expr = [](std::size_t value) -> expression
    {
        return expression_numeric_literal{.value = std::to_string(value)};
    };

    auto bits_expr = [](type_symbol of_type) -> expression
    {
        return expression_bits{.of_type = std::move(of_type)};
    };

    auto is_integral_expr = [](type_symbol of_type) -> expression
    {
        return expression_is_integral{.of_type = std::move(of_type)};
    };

    auto is_signed_expr = [](type_symbol of_type) -> expression
    {
        return expression_is_signed{.of_type = std::move(of_type)};
    };

    auto same_types_expr = [](type_symbol lhs_type, type_symbol rhs_type) -> expression
    {
        return expression_same_types{.lhs_type = std::move(lhs_type), .rhs_type = std::move(rhs_type)};
    };

    auto binary_expr = [](std::string operator_str, expression lhs, expression rhs) -> expression
    {
        return expression_binary{.operator_str = std::move(operator_str), .lhs = std::move(lhs), .rhs = std::move(rhs)};
    };

    auto static_choose_expr = [](expression condition, expression true_expr, expression false_expr) -> expression
    {
        return expression_static_choose{.condition = std::move(condition), .true_expr = std::move(true_expr), .false_expr = std::move(false_expr)};
    };


    if (typeis< readonly_constant >(input))
    {
        auto const& rc = input.get_as< readonly_constant >();
        if (rc.kind == constant_kind::string)
        {
            add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", string_literal_reference{}}}, void_type{});
        }
        if (rc.kind == constant_kind::numeric)
        {
            add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", numeric_literal_reference{}}}, void_type{});
        }
    }

    if (typeis< ptrref_type >(input) && input.get_as< ptrref_type >().ptr_class != pointer_class::ref)
    {
        // Pointer copy/default
        // We should onlt do this if it's *not* a reference (otherwise every refernece type is constructible from a const ref).
        add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", make_cref(input)}}, void_type{});
        add_overload({}, {{"THIS", create_nslot(input)}}, void_type{});
    }

    if (typeis_oneof< int_type, bool_type, readonly_constant, byte_type >(input))
    {
        add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", make_cref(input)}}, void_type{});
        if (typeis< byte_type >(input))
        {
            add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", numeric_literal_reference{}}}, void_type{});

            auto u8_type = type_symbol(int_type{.bits = 8, .has_sign = false});
            add_overload({}, {{"THIS", create_nslot(input)}, {"EXPLICIT", u8_type}}, void_type{});
        }
        else if (typeis< int_type >(input))
        {
            add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", numeric_literal_reference{}}}, void_type{});
            add_overload({}, {{"THIS", create_nslot(input)}, {"CHECKED", numeric_literal_reference{}}}, void_type{});
            add_overload({}, {{"THIS", create_nslot(input)}, {"ASSUME", numeric_literal_reference{}}}, void_type{});
            add_overload({}, {{"THIS", create_nslot(input)}, {"PARTIAL", numeric_literal_reference{}}}, void_type{});

            bool input_signed = input.as<int_type>().has_sign;
            std::size_t bits = input.as<int_type>().bits;

            auto integral_template = auto_temploidic{.name = "__int_type"};
            auto integral_guard_type = type_symbol(freebound_identifier{.name = "__int_type"});
            auto same_type_guard = same_types_expr(integral_guard_type, input);
            auto integral_guard = is_integral_expr(integral_guard_type);
            auto byte_guard = same_types_expr(integral_guard_type, type_symbol(byte_type{}));
            auto signed_guard = is_signed_expr(integral_guard_type);
            auto bits_of_input = bits_expr(integral_guard_type);
            auto bits_literal = int_literal_expr(bits);

            expression implicit_guard;
            if (input_signed)
            {
                implicit_guard = static_choose_expr(
                    same_type_guard,
                    false_expr(),
                    static_choose_expr(
                        integral_guard,
                        static_choose_expr(
                            byte_guard,
                            false_expr(),
                            static_choose_expr(
                                signed_guard,
                                binary_expr("<=", bits_of_input, bits_literal),
                                binary_expr("<", bits_of_input, bits_literal))),
                        false_expr()));
            }
            else
            {
                implicit_guard = static_choose_expr(
                    same_type_guard,
                    false_expr(),
                    static_choose_expr(
                        integral_guard,
                        static_choose_expr(
                            byte_guard,
                            false_expr(),
                            static_choose_expr(
                                signed_guard,
                                false_expr(),
                                binary_expr("<=", bits_of_input, bits_literal))),
                        false_expr()));
            }

            expression checked_guard = static_choose_expr(same_type_guard, false_expr(), integral_guard);
            expression assume_guard = static_choose_expr(same_type_guard, false_expr(), integral_guard);
            expression partial_guard = static_choose_expr(
                same_type_guard,
                false_expr(),
                static_choose_expr(
                    integral_guard,
                    binary_expr(">=", bits_of_input, bits_literal),
                    false_expr()));

            if (!input.type_is<byte_type>())
            {
                add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", integral_template}}, void_type{}, std::move(implicit_guard), -1);
            }
            add_overload({}, {{"THIS", create_nslot(input)}, {"CHECKED", integral_template}}, void_type{}, std::move(checked_guard), -1);
            add_overload({}, {{"THIS", create_nslot(input)}, {"ASSUME", integral_template}}, void_type{}, std::move(assume_guard), -1);
            add_overload({}, {{"THIS", create_nslot(input)}, {"PARTIAL", integral_template}}, void_type{}, std::move(partial_guard), -1);

            auto u8_type = type_symbol(int_type{.bits = 8, .has_sign = false});
            if (input == u8_type)
            {
                add_overload({}, {{"THIS", create_nslot(input)}, {"EXPLICIT", type_symbol(byte_type{})}}, void_type{});
            }
        }
        add_overload({}, {{"THIS", create_nslot(input)}}, void_type{});
    }

    if (typeis< ptrref_type >(input))
    {
        auto const& target_pref = input.get_as< ptrref_type >();

        std::set< pointer_class > allowed_input_classes;

        std::set< qualifier > allowed_qualifiiers;

        if (target_pref.ptr_class == pointer_class::ref)
        {
            allowed_input_classes.insert(pointer_class::ref);
        }

        if (target_pref.ptr_class == pointer_class::instance)
        {
            allowed_input_classes.insert(pointer_class::instance);
            allowed_input_classes.insert(pointer_class::array);
        }

        if (target_pref.ptr_class == pointer_class::array)
        {
            allowed_input_classes.insert(pointer_class::array);
        }

        // TODO: Decide machine pointer semantics

        if (target_pref.qual == qualifier::mut)
        {
            allowed_qualifiiers.insert(qualifier::mut);
            allowed_qualifiiers.insert(qualifier::temp);
        }
        else if (target_pref.qual == qualifier::constant)
        {
            allowed_qualifiiers.insert(qualifier::constant);
            allowed_qualifiiers.insert(qualifier::temp);
            allowed_qualifiiers.insert(qualifier::mut);
        }
        else if (target_pref.qual == qualifier::temp)
        {
            allowed_qualifiiers.insert(qualifier::temp);
        }
        else if (target_pref.qual == qualifier::write)
        {
            allowed_qualifiiers.insert(qualifier::write);
            allowed_qualifiiers.insert(qualifier::mut);
        }

        // input/output/auto are not concrete types so don't have constructors.

        if (target_pref.ptr_class == pointer_class::ref)
        {
            add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", input}}, void_type{});
        }

        for (qualifier q : allowed_qualifiiers)
        {
            for (pointer_class p : allowed_input_classes)
            {
                type_symbol type = ptrref_type{.target = target_pref.target, .ptr_class = p, .qual = q};

                // We don't want to generate a copy constructor here, this is only for conversions.
                if (type == input)
                {
                    continue;
                }

                add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", type}}, void_type{});
            }
        }

        if (target_pref.ptr_class == pointer_class::ref && qualifier_template_match(target_pref.qual, qualifier::temp).has_value())
        {
            // Reference materialization from a value follows the same constructor-based path
            // as other builtin conversions during codegen lowering.
            auto materialized_target = target_pref.target;
            if (typeis< nvalue_slot >(materialized_target))
            {
                materialized_target = as< nvalue_slot >(materialized_target).target;
            }
            add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", materialized_target}}, void_type{});
        }
    }

    if (typeis< procedure_type >(input))
    {
        co_return result;
    }

    if (typeis< int_type >(input) || input.type_is< bool_type >() || input.type_is< ptrref_type >() || input.type_is< readonly_constant >())
    {
        co_return result;
    }

    bool should_autogen_constructor = co_await rpnx::querygraph::request< class_requires_gen_default_ctor_query >(input);
    bool should_autogen_copy_constructor = co_await rpnx::querygraph::request< class_requires_gen_copy_ctor_query >(input);
    bool should_autogen_move_constructor = co_await rpnx::querygraph::request< class_requires_gen_move_ctor_query >(input);

    // co_await rpnx::querygraph::request< class_should_autogen_default_constructor_query >(input);

    if (should_autogen_constructor)
    {
        add_overload({}, {{"THIS", create_nslot(input)}}, void_type{});
        // co_return result;
    }

    if (should_autogen_copy_constructor)
    {
        add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", make_cref(input)}}, void_type{});
    }

    if (should_autogen_move_constructor)
    {
        add_overload({}, {{"THIS", create_nslot(input)}, {"OTHER", make_tref(input)}}, void_type{});
    }

    co_return result;
}
