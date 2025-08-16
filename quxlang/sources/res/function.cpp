// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/res/list_builtin_functum_overloads_resolver.hpp"
#include "quxlang/variant_utils.hpp"
#include <quxlang/macros.hpp>
#include <quxlang/res/function.hpp>
#include <quxlang/res/functum.hpp>

using namespace quxlang;

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_positional_parameter_names)
{
    std::vector< std::optional< std::string > > result;
    auto const& func = co_await QUX_CO_DEP(function_declaration, (input_val));

    if (!func.has_value())
    {
        throw std::logic_error("Function not found");
    }

    std::set< std::string > names;

    for (auto const& param : func->header.call_parameters)
    {
        if (param.api_name.has_value())
        {
            // non-positional parameter
            continue;
        }

        if (param.name.has_value() && names.contains(*param.name))
        {
            throw std::logic_error("Duplicate parameter name");
        }

        result.push_back(param.name);
    }

    QUX_CO_ANSWER(result);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_initialize)
{
    auto selection = co_await QUX_CO_DEP(functum_select_function, (input_val));

    if (!selection)
    {
        QUX_WHY("No function found that matches the given parameters.");

        QUX_CO_ANSWER(std::nullopt);
        // throw std::logic_error("No function found that matches the given parameters.");
    }

    co_return co_await QUX_CO_DEP(function_instanciation, (initialization_reference{.initializee = selection.value(), .parameters = input_val.parameters}));
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_primitive_constructors)
{
    std::set< builtin_function_info > result;

    if (typeis< int_type >(input) || input.type_is< bool_type >() || input.type_is< pointer_type >())
    {

        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{.type = create_nslot(input)}}, {"OTHER", argif{.type = make_cref(input)}}}}}, .return_type = void_type{}});
        if (input.type_is< int_type >())
        {
            result.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{.type = create_nslot(input)}}, {"OTHER", argif{.type = numeric_literal_reference{}}}}}}, .return_type = void_type{}});
        }
        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{.type = create_nslot(input)}}}}}, .return_type = void_type{}});
        co_return (result);
    }

    bool should_autogen_constructor = co_await QUX_CO_DEP(class_requires_gen_default_ctor, (input));
    bool should_autogen_copy_constructor = co_await QUX_CO_DEP(class_requires_gen_copy_ctor, (input));
    bool should_autogen_move_constructor = co_await QUX_CO_DEP(class_requires_gen_move_ctor, (input));

    // co_await QUX_CO_DEP(class_should_autogen_default_constructor, (input));

    if (should_autogen_constructor)
    {
        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{.type = create_nslot(input)}}}}}, .return_type = void_type{}});
        // co_return result;
    }

    if (should_autogen_copy_constructor)
    {
        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{.type = create_nslot(input)}}, {"OTHER", argif{.type = make_cref(input)}}}}}, .return_type = void_type{}});
    }

    if (should_autogen_move_constructor)
    {
        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{.type = create_nslot(input)}}, {"OTHER", argif{.type = make_tref(input)}}}}}, .return_type = void_type{}});
    }

    co_return result;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_primitive_overloads)
{
    auto const& functum = input;
    std::optional< type_symbol > parent_opt;
    std::string input_name = to_string(input);

    std::optional< ast2_class_declaration > class_ent;

    if (!typeis< submember >(functum))
    {
        co_return {};
    }

    submember const& as_submember = as< submember >(functum);
    type_symbol const& parent = as_submember.of;

    std::string const& name = as_submember.name;

    std::set< builtin_function_info > allowed_operations;

    if (name == "CONSTRUCTOR")
    {
        co_return co_await QUX_CO_DEP(list_primitive_constructors, (parent));
    }

    if (name == "OPERATOR??" && (parent.test< pointer_type >(
                                     [](pointer_type p)
                                     {
                                         return p.ptr_class != pointer_class::ref;
                                     }) ||
                                 parent.type_is< int_type >()))
    {
        builtin_function_info bl_info;

        bl_info.overload = temploid_ensig{.interface = {.named = {{"THIS", {parent}}}}};

        bl_info.return_type = bool_type{};

        allowed_operations.insert(bl_info);
    }

    auto uintptr_type = co_await QUX_CO_DEP(uintpointer_type, ({}));

    if (name == "OPERATOR[]" && parent.type_is< array_type >())
    {
        static std::vector< qualifier > quals{qualifier::mut, qualifier::constant, qualifier::mut, qualifier::temp, qualifier::write};

        for (qualifier qv : quals)
        {
            builtin_function_info br_info;
            br_info.overload = temploid_ensig{.interface = intertype{
                                                  .positional = {argif{.type = uintptr_type}},
                                                  .named = {{"THIS", argif{pointer_type{.target = parent, .ptr_class = pointer_class::ref, .qual = qualifier::constant}}}},
                                              }};
            br_info.return_type = pointer_type{.target = parent.get_as< array_type >().element_type, .ptr_class = pointer_class::ref, .qual = qv};

            allowed_operations.insert(br_info);
        }
    }

    if (name == "OPERATOR[&]" && parent.type_is< array_type >())
    {

        static std::vector< qualifier > quals{qualifier::mut, qualifier::constant, qualifier::mut, qualifier::temp, qualifier::write};

        for (qualifier qv : quals)
        {
            builtin_function_info br_info;
            br_info.overload = temploid_ensig{.interface = intertype{.positional = {argif{.type = uintptr_type}}, .named = {{"THIS", argif{pointer_type{.target = parent, .ptr_class = pointer_class::ref, .qual = qualifier::constant}}}}}};
            br_info.return_type = pointer_type{.target = parent.get_as< array_type >().element_type, .ptr_class = pointer_class::array, .qual = qv};

            allowed_operations.insert(br_info);
        }
    }

    // TODO: Add support for OPERATOR[] and OPERATOR[&] for array pointers

    if (name.starts_with("OPERATOR"))
    {
        std::string operator_name = name.substr(8);
        bool is_rhs = false;

        if (operator_name.ends_with("RHS"))
        {
            operator_name = operator_name.substr(0, operator_name.size() - 3);
            is_rhs = true;
        }

        bool is_int_type = typeis< int_type >(parent);
        bool is_bool_type = typeis< bool_type >(parent);
        bool is_pointer_type = typeis< pointer_type >(parent);
        bool is_arithmetic_operator = arithmetic_operators.contains(operator_name);
        bool is_swap_operator = operator_name == "<->";
        bool is_assignment_operator = base_operators.contains(operator_name);
        bool is_compare_operator = compare_operators.contains(operator_name);
        bool is_incdec_operator = incdec_operators.contains(operator_name);
        bool is_pointer_arith_operator = pointer_arithmetic_operators.contains(operator_name);

        if (is_swap_operator && (is_int_type || is_bool_type || is_pointer_type))
        {
            if (is_rhs)
            {
                // no primitive RHS swaps exist.
                co_return {};
            }
            allowed_operations.insert(builtin_function_info{
                .overload = temploid_ensig{.interface =
                                               intertype{
                                                   .named = {{"THIS", argif{make_mref(parent)}}, {"OTHER", argif{make_mref(parent)}}},
                                               }},
                .return_type = void_type{},
            });
        }

        if (is_int_type)
        {
            if (is_arithmetic_operator)
            {
                allowed_operations.insert(builtin_function_info{.overload =
                                                                    temploid_ensig{
                                                                        .interface =
                                                                            intertype{
                                                                                .named = {{"THIS", argif{parent}}, {"OTHER", argif{parent}}},
                                                                            },
                                                                    },
                                                                .return_type = parent});
            }
            else if (is_compare_operator)
            {
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{parent}}, {"OTHER", argif{parent}}}}}, .return_type = bool_type{}});
            }
            else if (is_incdec_operator && !is_rhs)
            {
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{make_mref(parent)}}}}}, .return_type = parent});
            }
            else if (is_incdec_operator && is_rhs)
            {
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{make_mref(parent)}}}}}, .return_type = make_mref(parent)});
            }
        }

        if (is_assignment_operator)
        {
            if (is_rhs)
            {
                // no primitive RHS assignments exist.
                co_return {};
            }
            else
            {
                allowed_operations.insert(builtin_function_info{
                    .overload = temploid_ensig{.interface =
                                                   intertype{
                                                       .named = {{"THIS", argif{make_wref(parent)}}, {"OTHER", argif{parent}}},
                                                   }},
                    .return_type = void_type{},
                });
            }
        }

        if (typeis< int_type >(parent) && compare_operators.contains(operator_name))
        {
            allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface =
                                                                                           intertype{
                                                                                               .named = {{"THIS", argif{parent}}, {"OTHER", argif{parent}}},
                                                                                           }},
                                                            .return_type = bool_type{}});
        }

        if (typeis< numeric_literal_reference >(parent) && arithmetic_operators.contains(operator_name))
        {
            std::set< builtin_function_info > allowed_operations;

            allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{numeric_literal_reference{}}}, {"OTHER", argif{numeric_literal_reference{}}}}}}, .return_type = numeric_literal_reference{}});
            co_return (allowed_operations);
        }

        if (typeis< pointer_type >(parent) && operator_name == rightarrow_operator)
        {
            allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{parent}}}}}, .return_type = make_mref(remove_ptr(parent))});
        }
        if (typeis< pointer_type >(parent))
        {
            pointer_type const& ptr = as< pointer_type >(parent);

            auto uintptr_type = co_await QUX_CO_DEP(uintpointer_type, (std::monostate{}));

            if (ptr.ptr_class == pointer_class::array && operator_name == "+" && !is_rhs)
            {
                // Arithmetic
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{ptr}}, {"OTHER", argif{uintptr_type}}}}}, .return_type = parent});
            }

            if (ptr.ptr_class == pointer_class::array && operator_name == "-" && !is_rhs)
            {
                // Arithmetic
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{ptr}}, {"OTHER", argif{uintptr_type}}}}}, .return_type = parent});

                // 2 pointer diff
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{ptr}}, {"OTHER", argif{ptr}}}}}, .return_type = uintptr_type});
            }

            if (ptr.ptr_class == pointer_class::array && incdec_operators.contains(operator_name))
            {
                if (!is_rhs)
                {
                    // Postfix
                    allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{make_mref(ptr)}}}}}, .return_type = parent});
                }
                else
                {
                    // prefix operator
                    allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{make_mref(ptr)}}}}}, .return_type = make_mref(parent)});
                }
            }
        }

        co_return (allowed_operations);
    }

    co_return allowed_operations;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_builtin)
{
    auto builtin_overloads = co_await QUX_CO_DEP(functum_builtin_overloads, (input_val.templexoid));

    for (auto const& info : builtin_overloads)
    {
        if (info == input_val.which)
        {
            co_return true;
        }
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_primitive)
{
    auto primitive_overloads = co_await QUX_CO_DEP(functum_primitive_overloads, (input_val.templexoid));

    for (auto const& info : primitive_overloads)
    {
        if (info.overload == input.which)
        {
            co_return info;
        }
    }

    co_return std::nullopt;
}


// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/res/function.hpp"

#include "../../../rpnx/include/rpnx/debug.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include <vector>

#include "quxlang/compiler.hpp"

using namespace quxlang;

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_ensig_initialize_with)
{
    auto os = input.ensig;
    auto args = input.params;
    // TODO: support default values for arguments

    auto val = this;


     std::string to = to_string(os.interface);
     std::string from = to_string(args);

    if (to == "INTERTYPE(MUT& MODULE(main)::buf)")
    {
        int debugger = 0;
    }

    if (os.interface.positional.size() != args.positional.size())
    {
        co_return std::nullopt;
    }

    if (os.interface.named.size() != args.named.size())
    {
        co_return std::nullopt;
    }

    std::vector< quxlang::compiler::out< bool > > convertibles_dp;


    for (auto const & [name, type] : args.named)
    {
        auto it = os.interface.named.find(name);
        if (it == os.interface.named.end())
        {
            co_return std::nullopt;
        }

        auto arg_type = type;
        auto param_type = it->second;

        std::string arg_type_str = to_string(arg_type);
        std::string param_type_str = to_string(param_type);

        // TODO: Default argument support.

        if (is_template(param_type.type))
        {
            auto match = match_template(param_type.type, arg_type);
            if (match.has_value())
            {
                continue;
            }
            else
            {
                co_return std::nullopt;
            }
        }
        else
        {
            convertibles_dp.push_back(c->lk_implicitly_convertible_to({arg_type, param_type.type}));
            add_co_dependency(convertibles_dp.back());
        }
    }

    for (int i = 0; i < os.interface.positional.size(); i++)
    {
        auto arg_type = args.positional.at(i);
        auto param_type = os.interface.positional.at(i);
        if (is_template(param_type.type))
        {

            QUXLANG_DEBUG(std::string arg_type_str = to_string(arg_type));
            QUXLANG_DEBUG(std::string param_type_str = to_string(param_type));

            auto tmatch = match_template(param_type.type, arg_type);
            if (tmatch.has_value())
            {
                continue;
            }
            else
            {
                co_return std::nullopt;
            }
        }
        else
        {
            convertibles_dp.push_back(c->lk_implicitly_convertible_to({arg_type, param_type.type}));

            add_co_dependency(convertibles_dp.back());
        }
    }

    invotype result;

    std::optional< invotype > result_opt;

    for (auto & dp : convertibles_dp)
    {
        auto is_convertible = co_await *dp;
        if (is_convertible == false)
        {
            co_return std::nullopt;
        }
    }

    for (auto const & [name, type] : args.named)
    {
        auto it = os.interface.named.find(name);
        if (it == os.interface.named.end())
        {
            co_return std::nullopt;
        }

        auto arg_type = type;
        auto param_type = it->second;

        if (is_template(param_type.type))
        {
            auto match = match_template(param_type.type, arg_type);
            assert(match); // checked already above

            result.named[name] = std::move(match.value().type);
        }
        else
        {
            auto dp = convertibles_dp.back();
            convertibles_dp.pop_back();
            auto arg_is_convertible = co_await *dp;

            if (arg_is_convertible == false)
            {
                co_return std::nullopt;
            }

            result.named[name] = param_type.type;
        }
    }

    for (std::size_t i = 0; i < args.positional.size(); i++)
    {
        auto arg_type = args.positional[i];
        auto param_type = os.interface.positional[i];

        if (is_template(param_type.type))
        {
            auto match = match_template(param_type.type, arg_type);
            assert(match); // checked already above

            result.positional.push_back(std::move(match.value().type));
        }
        else
        {
            auto dp = convertibles_dp[i];
            auto arg_is_convertible = co_await *dp;

            if (arg_is_convertible == false)
            {
                co_return std::nullopt;
            }

            result.positional.push_back(param_type.type);
        }
    }

    result_opt = result;
    co_return result_opt;
}



QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_select_function)
{


    auto input_str = to_string(input_val);

    if (typeis< temploid_reference >(input.initializee))
    {
        // TODO: We should identify a real match and error if this isn't a valid selection.
        // E.g. if there are type aliases, we should return the "real" type here instead of the type alias.
        // There should also be a selection error when this selection doesn't exist.
        // e.g. ::myint ALIAS I32;
        // ::foo FUNCTION(%x I32) ...
        // Would result in the following selection:
        // calle=foo#[::myint] params=(...) -> foo#[I32]

        QUX_CO_ANSWER(as< temploid_reference >(input.initializee));
    }

    auto sym_kind = co_await QUX_CO_DEP(symbol_type, (input.initializee));

    if (sym_kind != symbol_kind::functum)
    {
        co_return std::nullopt;
    }

    auto overloads = co_await QUX_CO_DEP(functum_overloads, (input.initializee));

    std::set< temploid_reference > best_match;
    std::optional< std::int64_t > highest_priority;

    for (auto const& o : overloads)
    {
        auto candidate = co_await QUX_CO_DEP(function_ensig_initialize_with, ({.ensig = o, .params = input.parameters}));
        if (candidate)
        {
            std::size_t priority = o.priority.value_or(0);

            if (!highest_priority || priority > *highest_priority)
            {
                highest_priority = priority;
                best_match.clear();
                best_match.insert({.templexoid = input.initializee, .which = o});
            }
            else if (priority == *highest_priority)
            {
                best_match.insert({.templexoid = input.initializee, .which = o});
            }
        }
    }

    if (best_match.size() == 0)
    {
        QUX_WHY("No matching overloads");
        QUX_CO_ANSWER(std::nullopt);
        // throw std::logic_error("No matching overloads");
    }
    else if (best_match.size() > 1)
    {
        QUX_WHY("Ambiguous overload");
        QUX_CO_ANSWER(std::nullopt);
    }

    QUX_CO_ANSWER(*best_match.begin());
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_exists_and_is_callable_with)
{
    auto ol = co_await QUX_CO_DEP(functum_initialize, (input_val));

    QUX_CO_ANSWER(ol.has_value());
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_builtin_overloads)
{
    auto const& primitive_overloads = co_await QUX_CO_DEP(functum_primitive_overloads, (input));

    std::set< temploid_ensig > results;

    for (auto const& info : primitive_overloads)
    {
        results.insert(info.overload);
    }

    // TODO: Add other builtin non-primitive overloads here.

    co_return results;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_map_user_formal_ensigs)
{
    auto const& decls = co_await QUX_CO_DEP(functum_list_user_ensig_declarations, (input));

    std::string input_name = quxlang::to_string(input);

    std::map< temploid_ensig, std::size_t > output;

    bool is_member_functum = typeis< submember >(input);
    std::optional<type_symbol> class_type;
    bool is_ctor = false;
    bool is_dtor = false;
    if (is_member_functum)
    {
        submember const& m = as< submember >(input);
        class_type = m.of;
        if (m.name == "CONSTRUCTOR")
        {
            is_ctor = true;
        }
        else if (m.name == "DESTRUCTOR")
        {
            is_dtor = true;
        }
    }

    for (std::size_t i = 0; i < decls.size(); i++)
    {
        auto const &decl = decls.at(i);
        temploid_ensig formal_ensig;
        formal_ensig.priority = decl.priority;
        for (auto const& param : decl.interface.named)
        {
            contextual_type_reference declared_type_with_context;
            declared_type_with_context.type = param.second.type;

            // We can't look at the typedefs of the function while we are resolving the function's formal ensig, as this would cause a circular dependency.
            declared_type_with_context.context = qualified_parent(input).value_or(void_type{});

            auto const& formal_type_opt = co_await QUX_CO_DEP(lookup, (declared_type_with_context));
            if (!formal_type_opt.has_value())
            {
                throw std::logic_error("Type not found");
            }
            formal_ensig.interface.named[param.first] = argif{.type = formal_type_opt.value(), .is_defaulted = param.second.is_defaulted};
        }
        for (auto const& param : decl.interface.positional)
        {
            contextual_type_reference declared_type_with_context;
            declared_type_with_context.type = param.type;
            declared_type_with_context.context = qualified_parent(input).value_or(void_type{});
            auto const& formal_type_opt = co_await QUX_CO_DEP(lookup, (declared_type_with_context));
            if (!formal_type_opt.has_value())
            {
                throw std::logic_error("Type not found");
            }
            formal_ensig.interface.positional.push_back(argif{.type = formal_type_opt.value(), .is_defaulted = param.is_defaulted});
        }

        if (is_member_functum && !formal_ensig.interface.named.contains("THIS"))
        {
            argif this_argif;

            if (is_ctor)
            {
                this_argif.type = nvalue_slot{.target = class_type.value()};
            }
            else if (is_dtor)
            {
                this_argif.type = dvalue_slot{.target = class_type.value()};
            }
            else
            {
                this_argif.type = pointer_type{.target = class_type.value(), .ptr_class = pointer_class::ref, .qual = qualifier::auto_};
            }

            formal_ensig.interface.named["THIS"] = this_argif;
        }

        if (output.contains(formal_ensig))
        {
            throw std::logic_error("Duplicate overload");
        }

        output.insert({formal_ensig, i});
    }

    co_return output;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_user_overloads)
{
    auto const& map = co_await QUX_CO_DEP(functum_map_user_formal_ensigs, (input));

    std::set< temploid_ensig > results;

    for (auto const& [ensig, index] : map)
    {
        results.insert(ensig);
    }

    co_return results;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_list_user_ensig_declarations)
{
    auto const& decls = co_await QUX_CO_DEP(functum_list_user_overload_declarations, (input));

    std::vector< temploid_ensig > output;

    for (std::size_t i = 0; i < decls.size(); i++)
    {
        auto const& head = decls.at(i).header;

        temploid_ensig ensig;
        ensig.priority = head.priority;

        for (std::size_t y = 0; y < head.call_parameters.size(); y++)
        {
            auto const& param = head.call_parameters.at(y);

            argif arg;
            if (param.default_expr.has_value())
            {
                arg.is_defaulted = true;
            }

            arg.type = param.type;

            if (param.api_name.has_value())
            {
                if (ensig.interface.named.contains(param.api_name.value()))
                {
                    throw std::logic_error("Duplicate parameter name");
                    //
                }

                ensig.interface.named[param.api_name.value()] = arg;
            }
            else
            {
                ensig.interface.positional.push_back(arg);
            }
        }

        output.push_back(ensig);
    }

    co_return output;
}
