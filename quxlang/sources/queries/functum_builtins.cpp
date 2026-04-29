// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_builtins_spec.hpp>
#include <quxlang/queries/template_builtin.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <quxlang/ast2/ast2_entity.hpp>
#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_builtins_spec > quxlang::functum_builtins_impl(type_symbol input)
{
    auto const& functum = input;
    std::optional< type_symbol > parent_opt;
    std::string input_name = to_string(input);

    std::optional< ast2_class_declaration > class_ent;

    std::set< builtin_function_info > allowed_operations;

    auto make_overload = [&](std::vector< type_symbol > positionals, std::map< std::string, type_symbol > named, type_symbol return_type)
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
        bl_info.return_type = return_type;
        return bl_info;
    };

    auto add_overload = [&](std::vector< type_symbol > positionals, std::map< std::string, type_symbol > named, type_symbol return_type)
    {
        allowed_operations.insert(make_overload(positionals, named, return_type));
    };

    auto uintptr_type = co_await rpnx::querygraph::request< uintpointer_type_query >({});

    if (typeis< builtin_symbol >(functum))
    {
        auto const& builtin = as< builtin_symbol >(functum);
        if (builtin.name == "SERIALIZE_UINTANY" || builtin.name == "SERIALIZE_LEB128")
        {
            add_overload({}, {{"VALUE", make_cref(auto_temploidic{.name = "__uint_type"})}, {"OUTPUT_ITERATOR", auto_temploidic{.name = "__out_iter"} }}, freebound_identifier{"__out_iter"});
        }
        else if (builtin.name == "DESERIALIZE_UINTANY" || builtin.name == "DESERIALIZE_LEB128")
        {
            add_overload({}, {{"VALUE", make_mref(auto_temploidic{.name = "__uint_type"})}, {"INPUT_ITERATOR", auto_temploidic{.name = "__in_iter"} }}, freebound_identifier{"__in_iter"});
        }
        co_return allowed_operations;
    }

    if (typeis< instanciation_reference >(functum))
    {
        auto const& inst = as< instanciation_reference >(functum);
        auto type_argument = inst.params.named.find("T");
        if (co_await rpnx::querygraph::request< template_builtin_query >(inst.temploid) && type_argument != inst.params.named.end() && inst.params.named.size() == 1 && inst.params.positional.empty())
        {
            if (!typeis< builtin_symbol >(inst.temploid.templexoid))
            {
                throw compiler_bug("builtin template selection did not have a builtin_symbol parent");
            }

            auto const& builtin = as< builtin_symbol >(inst.temploid.templexoid);
            auto allocator_kind = builtin_allocator_kind_from_name(builtin.name);
            if (!allocator_kind.has_value())
            {
                throw compiler_bug("builtin template selection had no allocator kind");
            }

            auto const allocated_type = parameter_instantiation_type(type_argument->second);
            auto const storage_type = type_symbol(storage{.storable_types = {allocated_type}});
            auto const single_ptr_type = type_symbol(ptrref_type{.target = storage_type, .ptr_class = pointer_class::instance, .qual = qualifier::mut});
            auto const multi_ptr_type = type_symbol(ptrref_type{.target = storage_type, .ptr_class = pointer_class::array, .qual = qualifier::mut});

            switch (*allocator_kind)
            {
            case builtin_allocator_kind::constexpr_alloc:
                add_overload({}, {}, single_ptr_type);
                break;
            case builtin_allocator_kind::constexpr_alloc_multiple:
                add_overload({uintptr_type}, {}, multi_ptr_type);
                break;
            case builtin_allocator_kind::constexpr_dealloc:
                add_overload({single_ptr_type}, {}, void_type{});
                break;
            case builtin_allocator_kind::constexpr_dealloc_multiple:
                add_overload({multi_ptr_type, uintptr_type}, {}, void_type{});
                break;
            }

            co_return allowed_operations;
        }
    }

    if (!typeis< submember >(functum))
    {
        co_return {};
    }

    ptrref_type byte_ptr_type = ptrref_type{.target = byte_type{}, .ptr_class = pointer_class::array, .qual = qualifier::constant};

    submember const& as_submember = as< submember >(functum);
    type_symbol const& parent = as_submember.of;

    std::string const& name = as_submember.name;

    auto parent_kind = co_await rpnx::querygraph::request< symbol_type_query >(parent);

    if (parent_kind == symbol_kind::global_variable)
    {
        auto variable_type = co_await rpnx::querygraph::request< variable_type_query >(parent);
        storage global_storage_type;
        global_storage_type.storable_types.insert(variable_type);

        if (name == "GET_REFERENCE")
        {
            auto is_antestatal_static = co_await rpnx::querygraph::request< global_is_antestatal_static_query >(parent);
            auto is_serialoid_static = co_await rpnx::querygraph::request< global_is_serialoid_static_query >(parent);
            auto is_string_static = co_await rpnx::querygraph::request< global_is_string_static_query >(parent);
            auto ref_type = (is_antestatal_static || is_serialoid_static || is_string_static) ? make_cref(variable_type) : make_mref(variable_type);
            add_overload({}, {}, ref_type);
            co_return allowed_operations;
        }

        if (name == "INIT")
        {
            add_overload({}, {{"STORAGE", make_mref(global_storage_type)}}, void_type{});
            co_return allowed_operations;
        }
    }

    if (name == "CONSTRUCTOR")
    {
        co_return co_await rpnx::querygraph::request< list_builtin_constructors_query >(parent);
    }

    if (typeis< constexpr_proxy >(parent))
    {
        if (name == "OPERATOR++" || name == "OPERATOR->")
        {
            add_overload({}, {{"THIS", make_mref(parent)}}, make_mref(parent));
            co_return allowed_operations;
        }
        if (name == "OPERATOR:=")
        {
            add_overload({}, {{"THIS", make_wref(parent)}, {"OTHER", parent}}, void_type{});
            add_overload({}, {{"THIS", make_wref(parent)}, {"OTHER", byte_type{}}}, void_type{});
            co_return allowed_operations;
        }
    }

    if (name == "OPERATOR??" && (parent.test< ptrref_type >(
                                     [](ptrref_type p)
                                     {
                                         return p.ptr_class != pointer_class::ref;
                                     }) ||
                                 parent.type_is< int_type >()))
    {
        add_overload({}, {{"THIS", parent}}, bool_type{});
    }

    auto sintptr_type = co_await rpnx::querygraph::request< sintpointer_type_query >({});

    if (name == "OPERATOR[]" && parent.type_is< array_type >())
    {
        static std::vector< qualifier > quals{qualifier::mut, qualifier::constant, qualifier::mut, qualifier::temp, qualifier::write};

        for (qualifier qv : quals)
        {
            add_overload({uintptr_type}, {{"THIS", ptrref_type{.target = parent, .ptr_class = pointer_class::ref, .qual = qv}}}, ptrref_type{.target = parent.get_as< array_type >().element_type, .ptr_class = pointer_class::ref, .qual = qv});
        }
    }

    if (name == "OPERATOR[&]" && parent.type_is< array_type >())
    {
        static std::vector< qualifier > quals{qualifier::mut, qualifier::constant, qualifier::mut, qualifier::temp, qualifier::write};
        for (qualifier qv : quals)
        {
            add_overload({uintptr_type}, {{"THIS", ptrref_type{.target = parent, .ptr_class = pointer_class::ref, .qual = qv}}}, ptrref_type{.target = parent.get_as< array_type >().element_type, .ptr_class = pointer_class::array, .qual = qv});
        }
    }

    if ((name == "OPERATOR[]" || name == "OPERATOR[&]") && parent.type_is< ptrref_type >())
    {
        auto const& ptr = parent.get_as< ptrref_type >();
        if (ptr.ptr_class == pointer_class::array)
        {
            if (name == "OPERATOR[]")
            {
                add_overload({uintptr_type}, {{"THIS", parent}}, ptrref_type{.target = ptr.target, .ptr_class = pointer_class::ref, .qual = ptr.qual});
                add_overload({sintptr_type}, {{"THIS", parent}}, ptrref_type{.target = ptr.target, .ptr_class = pointer_class::ref, .qual = ptr.qual});
            }
            else
            {
                add_overload({uintptr_type}, {{"THIS", parent}}, parent);
                add_overload({sintptr_type}, {{"THIS", parent}}, parent);
            }
        }
    }

    if (parent.type_is< readonly_constant >() && (name == "BEGIN" || name == "END"))
    {
        add_overload({}, {{"THIS", make_cref(parent)}}, byte_ptr_type);
        co_return allowed_operations;
    }

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
        bool is_byte_type = typeis< byte_type >(parent);
        bool is_bool_type = typeis< bool_type >(parent);
        bool is_pointer_type = typeis< ptrref_type >(parent);
        bool is_arithmetic_operator = arithmetic_operators.contains(operator_name);
        bool is_compound_assignment_operator = compound_assignment_operators.contains(operator_name);
        bool is_swap_operator = operator_name == "<->";
        bool is_assignment_operator = operator_name == ":=";
        bool is_compare_operator = compare_operators.contains(operator_name);
        bool is_incdec_operator = incdec_operators.contains(operator_name);
        bool is_pointer_arith_operator = pointer_arithmetic_operators.contains(operator_name);
        bool is_regular_primitive_type = is_int_type || is_bool_type || is_pointer_type || is_byte_type;
        if (is_swap_operator && is_regular_primitive_type)
        {
            if (is_rhs)
            {
                // no primitive RHS swaps exist.
                co_return {};
            }
            add_overload({}, {{"THIS", make_mref(parent)}, {"OTHER", make_mref(parent)}}, void_type{});
        }
        else if (is_swap_operator)
        {
            bool should_autogen_swap = co_await rpnx::querygraph::request< class_requires_gen_swap_query >(parent);
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("Should autogen swap for {}: {}", to_string(parent), should_autogen_swap ? "yes" : "no");
            }
            if (should_autogen_swap && !is_rhs)
            {
                add_overload({}, {{"THIS", make_mref(parent)}, {"OTHER", make_mref(parent)}}, void_type{});
            }
        }

        if (is_int_type || is_byte_type)
        {
            if (is_compound_assignment_operator)
            {
                if (!is_rhs)
                {
                    auto const base_operator = compound_assignment_operators.at(operator_name);
                    static const std::set< std::string > bitwise_shift_operators = {"#++", "#--"};
                    static const std::set< std::string > bitwise_rotate_operators = {"#+%", "#-%"};
                    if (arithmetic_operators.contains(base_operator))
                    {
                        add_overload({}, {{"THIS", make_mref(parent)}, {"OTHER", parent}}, void_type{});
                    }
                    else if (bitwise_shift_operators.contains(base_operator) || bitwise_rotate_operators.contains(base_operator))
                    {
                        add_overload({}, {{"THIS", make_mref(parent)}, {"OTHER", uintptr_type}}, void_type{});
                    }
                    else
                    {
                        add_overload({}, {{"THIS", make_mref(parent)}, {"OTHER", parent}}, void_type{});
                    }
                }
            }
            else if (is_arithmetic_operator)
            {
                add_overload({}, {{"THIS", parent}, {"OTHER", parent}}, parent);
            }
            else if (is_compare_operator)
            {
                add_overload({}, {{"THIS", parent}, {"OTHER", parent}}, bool_type{});
            }
            else if (is_incdec_operator && !is_rhs)
            {
                add_overload({}, {{"THIS", make_mref(parent)}}, parent);
            }
            else if (is_incdec_operator && is_rhs)
            {
                add_overload({}, {{"THIS", make_mref(parent)}}, make_mref(parent));
            }
            else
            {
                // Bitwise operators support per docs/operators_syntax.md
                static const std::set< std::string > bitwise_binary_operators = {"#&&", "#||", "#&!", "#|!", "#^>", "#^<", "#^^", "#^!"};
                static const std::set< std::string > bitwise_shift_operators = {"#++", "#--"};
                static const std::set< std::string > bitwise_rotate_operators = {"#+%", "#-%"};

                if (bitwise_binary_operators.contains(operator_name))
                {
                    // (THIS parent, OTHER parent): parent
                    add_overload({}, {{"THIS", parent}, {"OTHER", parent}}, parent);
                }
                else if (bitwise_shift_operators.contains(operator_name))
                {
                    // Shifts: (THIS parent, OTHER uintptr): parent
                    add_overload({}, {{"THIS", parent}, {"OTHER", uintptr_type}}, parent);
                }
                else if (bitwise_rotate_operators.contains(operator_name))
                {
                    // Rotate: (THIS parent, OTHER uintptr): parent
                    add_overload({}, {{"THIS", parent}, {"OTHER", uintptr_type}}, parent);
                }
                else if (operator_name == "#!!" && !is_rhs)
                {
                    // Unary bitwise inverse/complement (suffix): (THIS parent) -> parent
                    add_overload({}, {{"THIS", parent}}, parent);
                }
            }
        }

        if (is_assignment_operator)
        {
            if (is_rhs)
            {
                // no builtin RHS assignments exist.
                co_return {};
            }
            else if (is_regular_primitive_type)
            {
                add_overload({}, {{"THIS", make_wref(parent)}, {"OTHER", parent}}, void_type{});
            }
            else
            {
                bool should_autogen_assignment = co_await rpnx::querygraph::request< class_requires_gen_assignment_query >(parent);
                if (should_autogen_assignment)
                {
                    add_overload({}, {{"THIS", make_wref(parent)}, {"OTHER", parent}}, void_type{});
                }
            }
        }

        if ((is_int_type || is_byte_type) && compare_operators.contains(operator_name))
        {
            add_overload({}, {{"THIS", parent}, {"OTHER", parent}}, bool_type{});
        }

        if (typeis< numeric_literal_reference >(parent) && arithmetic_operators.contains(operator_name))
        {
            add_overload({}, {{"THIS", numeric_literal_reference{}}, {"OTHER", numeric_literal_reference{}}}, numeric_literal_reference{});
        }
        else if (typeis< numeric_literal_reference >(parent) && compare_operators.contains(operator_name))
        {
            add_overload({}, {{"THIS", numeric_literal_reference{}}, {"OTHER", numeric_literal_reference{}}}, bool_type{});
        }

        if (typeis< ptrref_type >(parent) && operator_name == rightarrow_operator)
        {
            auto ptr = as< ptrref_type >(parent);
            add_overload({}, {{"THIS", parent}}, ptrref_type{.target = remove_ptr(parent), .ptr_class = pointer_class::ref, .qual = ptr.qual});
        }

        if (operator_name == "()" && typeis< procedure_type >(parent) && !is_rhs)
        {
            auto const& proc = as< procedure_type >(parent);
            auto named = proc.signature.params.named;
            named["THIS"] = make_cref(parent);
            add_overload(proc.signature.params.positional, named, proc.signature.return_type.value_or(type_symbol(void_type{})));
        }

        if (operator_name == "()" && typeis< ptrref_type >(parent) && !is_rhs)
        {
            auto const& ptr = as< ptrref_type >(parent);
            if (ptr.ptr_class == pointer_class::instance && typeis< procedure_type >(ptr.target))
            {
                auto const& proc = as< procedure_type >(ptr.target);
                auto named = proc.signature.params.named;
                named["THIS"] = make_cref(parent);
                add_overload(proc.signature.params.positional, named, proc.signature.return_type.value_or(type_symbol(void_type{})));
            }
        }

        if (!is_rhs && basic_compare_operators.contains(operator_name) && !is_regular_primitive_type)
        {
            auto user_defined_operator = co_await rpnx::querygraph::request< functum_user_overloads_query >(submember{.of = parent, .name = "OPERATOR" + operator_name});
            if (user_defined_operator.empty() && co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(parent))
            {
                add_overload({}, {{"THIS", make_cref(parent)}, {"OTHER", make_cref(parent)}}, bool_type{});
            }
        }

        // Bools use the regular 0/1 ordering so FALSE < TRUE.
        if (is_bool_type && compare_operators.contains(operator_name))
        {
            add_overload({}, {{"THIS", parent}, {"OTHER", parent}}, bool_type{});
        }
        else if (is_bool_type)
        {
            static const std::set< std::string > bool_binary_logic_operators = {"&&", "||", "^^", "^!", "&!", "|!", "^>", "^<"};

            if (bool_binary_logic_operators.contains(operator_name))
            {
                add_overload({}, {{"THIS", parent}, {"OTHER", parent}}, bool_type{});
            }
            else if (operator_name == "!!" && !is_rhs)
            {
                add_overload({}, {{"THIS", parent}}, bool_type{});
            }
        }

        if (typeis< ptrref_type >(parent))
        {
            ptrref_type const& ptr = as< ptrref_type >(parent);

            if (ptr.ptr_class != pointer_class::ref && basic_compare_operators.contains(operator_name) && !is_rhs)
            {
                add_overload({}, {{"THIS", ptr}, {"OTHER", ptr}}, bool_type{});
            }

            if (ptr.ptr_class == pointer_class::array && relative_compare_operators.contains(operator_name) && !is_rhs)
            {
                add_overload({}, {{"THIS", ptr}, {"OTHER", ptr}}, bool_type{});
            }

            if (ptr.ptr_class == pointer_class::array && operator_name == "+" && !is_rhs)
            {
                // Arithmetic
                add_overload({}, {{"THIS", ptr}, {"OTHER", uintptr_type}}, parent);
                add_overload({}, {{"THIS", ptr}, {"OTHER", sintptr_type}}, parent);
            }

            if (ptr.ptr_class == pointer_class::array && operator_name == "-" && !is_rhs)
            {
                // Arithmetic
                add_overload({}, {{"THIS", ptr}, {"OTHER", uintptr_type}}, parent);
                add_overload({}, {{"THIS", ptr}, {"OTHER", sintptr_type}}, parent);

                // 2 pointer diff
                add_overload({}, {{"THIS", ptr}, {"OTHER", ptr}}, sintptr_type);
            }

            if (ptr.ptr_class == pointer_class::array && incdec_operators.contains(operator_name))
            {
                if (!is_rhs)
                {
                    // Postfix
                    add_overload({}, {{"THIS", make_mref(ptr)}}, parent);
                }
                else
                {
                    // prefix operator
                    add_overload({}, {{"THIS", make_mref(ptr)}}, make_mref(parent));
                }
            }
        }

        co_return (allowed_operations);
    }

    if (name == "SERIALIZE" && (co_await rpnx::querygraph::request< functum_user_overloads_query >(input)).empty() && co_await rpnx::querygraph::request< type_should_autogen_serialize_query >(parent))
    {
        add_overload({}, {{"THIS", make_cref(parent)}, {"OUTPUT_ITERATOR", auto_temploidic{.name = "__out_iter"} }}, freebound_identifier{"__out_iter"});
    }
    else if (name == "DESERIALIZE" && (co_await rpnx::querygraph::request< functum_user_overloads_query >(input)).empty() && co_await rpnx::querygraph::request< type_should_autogen_deserialize_query >(parent))
    {
        add_overload({}, {{"THIS", make_wref(parent)}, {"INPUT_ITERATOR", auto_temploidic{.name = "__in_iter"} }}, freebound_identifier{"__in_iter"});
    }

    co_return allowed_operations;
}
