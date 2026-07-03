// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/profiling.hpp"

#include <quxlang/queries/specs/list_builtin_constructors_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <cstdint>
#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"


#include <quxlang/macros.hpp>

using namespace quxlang;

rpnx::querygraph::coroutine< quxlang::list_builtin_constructors_spec > quxlang::list_builtin_constructors_impl(type_symbol input)
{
    if (typeis< nvalue_slot >(input))
    {
        input = as< nvalue_slot >(input).target;
    }
    else if (typeis< dvalue_slot >(input))
    {
        input = as< dvalue_slot >(input).target;
    }

    std::set< builtin_function_info > result;
    if (typeis< thistype >(input))
    {
        co_return result;
    }
    type_symbol const builtin_self_type = thistype{};

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
        builtin_function_info ol;
        run_under_profiling_void(
            [&]
            {
                return "list_builtin_constructors make_overload";
            },
            [&]
            {
                ol = make_overload(positionals, named, return_type, std::move(enable_if), priority);
            });
        run_under_profiling_void(
            [&]
            {
                return "list_builtin_constructors insert '" + quxlang::to_string(ol.overload.interface);
            },
            [&]
            {
                result.insert(ol);
            });
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

    auto numeric_literal_is_zero = [](expression const& expr) -> bool
    {
        if (!expr.template type_is< expression_numeric_literal >())
        {
            return false;
        }
        auto const& value = expr.template get_as< expression_numeric_literal >().value;
        for (char ch : value)
        {
            if (ch != '0')
            {
                return false;
            }
        }
        return true;
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

    auto numeric_literal_fits_expr = [](type_symbol literal_type, type_symbol target_type) -> expression
    {
        return expression_numeric_literal_fits{.literal_type = std::move(literal_type), .target_type = std::move(target_type)};
    };

    auto binary_expr = [](std::string operator_str, expression lhs, expression rhs) -> expression
    {
        return expression_binary{.operator_str = std::move(operator_str), .lhs = std::move(lhs), .rhs = std::move(rhs)};
    };

    auto static_choose_expr = [](expression condition, expression true_expr, expression false_expr) -> expression
    {
        return expression_static_choose{.condition = std::move(condition), .true_expr = std::move(true_expr), .false_expr = std::move(false_expr)};
    };

    auto is_void_type = [](type_symbol const& type) -> bool
    {
        return typeis< void_type >(type);
    };

    if (typeis< builtin_symbol >(input) && is_builtin_atomic_access_mode_name(as< builtin_symbol >(input).name))
    {
        co_return result;
    }

    symbol_kind const input_kind = co_await rpnx::querygraph::request< symbol_type_query >(input);

    if (input_kind == symbol_kind::interface_)
    {
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_cref(builtin_self_type)}}, void_type{});
                                 });
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_tref(builtin_self_type)}}, void_type{});
                                 });
        if (co_await rpnx::querygraph::request< interface_defaultable_query >(input))
        {
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}}, void_type{});
                                     });
        }
        co_return result;
    }

    if (input_kind == symbol_kind::enum_)
    {
        enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(input);
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_cref(builtin_self_type)}}, void_type{});
                                 });
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_tref(builtin_self_type)}}, void_type{});
                                 });
        if (info.default_value_name.has_value())
        {
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}}, void_type{});
                                     });
        }
        co_return result;
    }

    if (input_kind == symbol_kind::flagset_)
    {
        flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(input);
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_cref(builtin_self_type)}}, void_type{});
                                 });
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_tref(builtin_self_type)}}, void_type{});
                                 });
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}}, void_type{});
                                 });

        std::vector< std::uint64_t > unsigned_widths{8, 16, 32, 64};
        for (std::uint64_t width : unsigned_widths)
        {
            run_under_profiling_void("list_builtin_constructors flagset width loop body",
                                     [&]
                                     {
                                         if (width >= info.bits)
                                         {
                                             run_under_profiling_void("list_builtin_constructors add_overload call",
                                                                      [&]
                                                                      {
                                                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"EXPLICIT", int_type{.bits = width, .has_sign = false}}}, void_type{});
                                                                      });
                                         }
                                     });
        }
        co_return result;
    }

    if (auto atomic_value_type = atomic_type_argument(input); atomic_value_type.has_value())
    {
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}}, void_type{});
                                 });
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_cref(*atomic_value_type)}}, void_type{});
                                 });
        co_return result;
    }

    if (typeis< readonly_constant >(input))
    {
        auto const& rc = input.get_as< readonly_constant >();
        if (rc.kind == constant_kind::string)
        {
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", string_literal_any_temploidic{.name = "__lit"}}}, void_type{});
                                     });
            // Explicit-only cast: NUMERIC_CONSTANT AS STRING_CONSTANT
            // Both share the same {__start, __end} byte-span layout, so it's a bitwise copy
            // via load_from_ref (same path as STRING_CONSTANT copy from a ref).
            // No @OTHER overload -> not implicit. No reverse -> STRING_CONSTANT AS NUMERIC_CONSTANT rejected.
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"EXPLICIT", make_cref(readonly_constant{.kind = constant_kind::numeric})}}, void_type{});
                                     });
        }
        if (rc.kind == constant_kind::numeric)
        {
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", numeric_literal_any_temploidic{.name = "__lit"}}}, void_type{});
                                     });
        }
    }

    if (typeis< ptrref_type >(input) && input.get_as< ptrref_type >().ptr_class != pointer_class::ref)
    {
        // Pointer copy/default
        // We should onlt do this if it's *not* a reference (otherwise every refernece type is constructible from a const ref).
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_cref(builtin_self_type)}}, void_type{});
                                 });
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}}, void_type{});
                                 });
    }

    if (typeis_oneof< int_type, float_type, bool_type, readonly_constant, byte_type >(input))
    {
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_cref(builtin_self_type)}}, void_type{});
                                 });
        if (typeis< byte_type >(input))
        {
            auto lit_any = numeric_literal_any_temploidic{.name = "__lit"};
            auto lit_guard_type = type_symbol(freebound_identifier{.name = "__lit"});
            auto fits_guard = numeric_literal_fits_expr(lit_guard_type, input);
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", lit_any}}, void_type{}, std::move(fits_guard), -1);
                                     });

            auto u8_type = type_symbol(int_type{.bits = 8, .has_sign = false});
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"EXPLICIT", u8_type}}, void_type{});
                                     });
        }
        else if (typeis< int_type >(input))
        {
            auto lit_any = numeric_literal_any_temploidic{.name = "__lit"};
            auto lit_guard_type = type_symbol(freebound_identifier{.name = "__lit"});
            auto fits_guard = numeric_literal_fits_expr(lit_guard_type, input);
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", lit_any}}, void_type{}, std::move(fits_guard), -1);
                                     });
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"CHECKED", lit_any}}, void_type{});
                                     });
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"ASSUME", lit_any}}, void_type{});
                                     });
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"PARTIAL", lit_any}}, void_type{});
                                     });

            bool input_signed = input.as< int_type >().has_sign;
            std::size_t bits = input.as< int_type >().bits;

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
                implicit_guard = static_choose_expr(same_type_guard, false_expr(), static_choose_expr(integral_guard, static_choose_expr(byte_guard, false_expr(), static_choose_expr(signed_guard, binary_expr("<=", bits_of_input, bits_literal), binary_expr("<", bits_of_input, bits_literal))), false_expr()));
            }
            else
            {
                implicit_guard = static_choose_expr(same_type_guard, false_expr(), static_choose_expr(integral_guard, static_choose_expr(byte_guard, false_expr(), static_choose_expr(signed_guard, false_expr(), binary_expr("<=", bits_of_input, bits_literal))), false_expr()));
            }

            expression checked_guard = static_choose_expr(same_type_guard, false_expr(), integral_guard);
            expression assume_guard = static_choose_expr(same_type_guard, false_expr(), integral_guard);
            expression partial_guard = static_choose_expr(same_type_guard, false_expr(), static_choose_expr(integral_guard, binary_expr(">=", bits_of_input, bits_literal), false_expr()));

            if (!input.type_is< byte_type >())
            {
                run_under_profiling_void("list_builtin_constructors add_overload call",
                                         [&]
                                         {
                                             add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", integral_template}}, void_type{}, std::move(implicit_guard), -1);
                                         });
            }
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"CHECKED", integral_template}}, void_type{}, std::move(checked_guard), -1);
                                     });
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"ASSUME", integral_template}}, void_type{}, std::move(assume_guard), -1);
                                     });
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"PARTIAL", integral_template}}, void_type{}, std::move(partial_guard), -1);
                                     });

            auto u8_type = type_symbol(int_type{.bits = 8, .has_sign = false});
            if (input == u8_type)
            {
                run_under_profiling_void("list_builtin_constructors add_overload call",
                                         [&]
                                         {
                                             add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"EXPLICIT", type_symbol(byte_type{})}}, void_type{});
                                         });
            }
        }
        else if (typeis< float_type >(input))
        {
            auto lit_any = numeric_literal_any_temploidic{.name = "__lit"};
            auto lit_guard_type = type_symbol(freebound_identifier{.name = "__lit"});
            auto fits_guard = numeric_literal_fits_expr(lit_guard_type, input);
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", lit_any}}, void_type{}, std::move(fits_guard), -1);
                                     });
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"APPROXIMATE", lit_any}}, void_type{});
                                     });

            auto const& float_info = input.as< float_type >();
            std::size_t const precision_bits = float_info.bits - float_info.exponent_bits;
            auto integral_template = auto_temploidic{.name = "__int_type"};
            auto integral_guard_type = type_symbol(freebound_identifier{.name = "__int_type"});
            auto exact_integer_guard = static_choose_expr(is_integral_expr(integral_guard_type), static_choose_expr(is_signed_expr(integral_guard_type), binary_expr("<=", bits_expr(integral_guard_type), int_literal_expr(precision_bits + 1)), binary_expr("<=", bits_expr(integral_guard_type), int_literal_expr(precision_bits))), false_expr());

            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", integral_template}}, void_type{}, std::move(exact_integer_guard), -1);
                                     });
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"APPROXIMATE", integral_template}}, void_type{}, is_integral_expr(integral_guard_type), -1);
                                     });
        }
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}}, void_type{});
                                 });
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

        if (target_pref.ptr_class == pointer_class::instance || target_pref.ptr_class == pointer_class::array)
        {
            bool const target_is_void = is_void_type(target_pref.target);
            for (qualifier q : allowed_qualifiiers)
            {
                run_under_profiling_void("list_builtin_constructors reinterpret qualifier loop body",
                                         [&]
                                         {
                                             if (target_is_void)
                                             {
                                                 type_symbol source_target = type_symbol(auto_temploidic{.name = "__reinterpret_pointee_type"});
                                                 type_symbol source_type = ptrref_type{.target = source_target, .ptr_class = target_pref.ptr_class, .qual = q};
                                                 run_under_profiling_void("list_builtin_constructors add_overload call",
                                                                          [&]
                                                                          {
                                                                              add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"REINTERPRET", source_type}}, void_type{});
                                                                          });
                                             }
                                             else
                                             {
                                                 type_symbol source_type = ptrref_type{.target = void_type{}, .ptr_class = target_pref.ptr_class, .qual = q};
                                                 run_under_profiling_void("list_builtin_constructors add_overload call",
                                                                          [&]
                                                                          {
                                                                              add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"REINTERPRET", source_type}}, void_type{});
                                                                          });
                                             }
                                         });
            }
        }

        if (target_pref.ptr_class == pointer_class::ref)
        {
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", builtin_self_type}}, void_type{});
                                     });
        }

        for (qualifier q : allowed_qualifiiers)
        {
            for (pointer_class p : allowed_input_classes)
            {
                run_under_profiling_void("list_builtin_constructors pointer conversion loop body",
                                         [&]
                                         {
                                             type_symbol type = ptrref_type{.target = target_pref.target, .ptr_class = p, .qual = q};

                                             // We don't want to generate a copy constructor here, this is only for conversions.
                                             if (type == input)
                                             {
                                                 return;
                                             }

                                             run_under_profiling_void("list_builtin_constructors add_overload call",
                                                                      [&]
                                                                      {
                                                                          add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", type}}, void_type{});
                                                                      });
                                         });
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
            run_under_profiling_void("list_builtin_constructors add_overload call",
                                     [&]
                                     {
                                         add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", materialized_target}}, void_type{});
                                     });
        }
    }

    if (typeis< array_type >(input))
    {
        auto const& array = input.get_as< array_type >();
        if (!numeric_literal_is_zero(array.element_count))
        {
            builtin_function_info bl_info;
            bl_info.overload.interface.named["THIS"] = argif{.type = create_nslot(builtin_self_type)};
            bl_info.overload.interface.positional.push_back(argif{.type = type_temploidic{}, .is_pack = true});
            bl_info.overload.enable_if = expression_value_keyword{.keyword = "TRUE"};
            bl_info.overload.priority = 0;
            bl_info.return_type = void_type{};
            run_under_profiling_void("list_builtin_constructors array pack insert",
                                     [&]
                                     {
                                         result.insert(std::move(bl_info));
                                     });
        }
    }

    if (typeis< procedure_type >(input))
    {
        co_return result;
    }

    if (typeis< int_type >(input) || typeis< float_type >(input) || input.type_is< bool_type >() || input.type_is< ptrref_type >() || input.type_is< readonly_constant >())
    {
        co_return result;
    }

    bool should_autogen_constructor = co_await rpnx::querygraph::request< class_requires_gen_default_ctor_query >(input);
    bool should_autogen_copy_constructor = co_await rpnx::querygraph::request< class_requires_gen_copy_ctor_query >(input);
    bool should_autogen_move_constructor = co_await rpnx::querygraph::request< class_requires_gen_move_ctor_query >(input);

    // co_await rpnx::querygraph::request< class_should_autogen_default_constructor_query >(input);

    if (should_autogen_constructor)
    {
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}}, void_type{});
                                 });
        // co_return result;
    }

    if (should_autogen_copy_constructor)
    {
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_cref(builtin_self_type)}}, void_type{});
                                 });
    }

    if (should_autogen_move_constructor)
    {
        run_under_profiling_void("list_builtin_constructors add_overload call",
                                 [&]
                                 {
                                     add_overload({}, {{"THIS", create_nslot(builtin_self_type)}, {"OTHER", make_tref(builtin_self_type)}}, void_type{});
                                 });
    }

    co_return result;
}
