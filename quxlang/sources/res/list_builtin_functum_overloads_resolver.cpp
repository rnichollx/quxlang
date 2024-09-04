//
// Created by Ryan Nicholl on 11/18/23.
//
#include "quxlang/res/list_builtin_functum_overloads_resolver.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/operators.hpp"

auto quxlang::list_builtin_functum_overloads_resolver::co_process(compiler* c, type_symbol functum) -> co_type
{
    std::optional< type_symbol > parent_opt;
    std::string name = to_string(functum);

    std::optional< ast2_class_declaration > class_ent;
    if (typeis< subdotentity_reference >(functum))
    {
        auto parent = as< subdotentity_reference >(functum).parent;

        auto parent_class_exists = co_await *c->lk_entity_canonical_chain_exists(parent);
        if (parent_class_exists)
        {
            auto decl = co_await QUX_CO_DEP(symboid, (parent));

            if (typeis< ast2_class_declaration >(decl))
            {
                class_ent = as< ast2_class_declaration >(decl);
            }
        }

        parent_opt = parent;
        auto name = as< subdotentity_reference >(functum).subdotentity_name;

        if (typeis< primitive_type_integer_reference >(parent) && name.starts_with("OPERATOR"))
        {
            auto int_type = as< primitive_type_integer_reference >(parent);
            std::set< primitive_function_info > allowed_operations;

            std::string operator_name = name.substr(8);
            bool is_rhs = false;
            if (operator_name.ends_with("RHS"))
            {
                operator_name = operator_name.substr(0, operator_name.size() - 3);

                is_rhs = true;
            }

            if (arithmetic_operators.contains(operator_name))
            {
                allowed_operations.insert(primitive_function_info{
                    .overload = function_overload{.builtin= true, .call_parameters = call_type{.named_parameters = {{"THIS", int_type}, {"OTHER", int_type}}, }, },
                    .return_type = int_type
                });
            }

            if (assignment_operators.contains(operator_name) && is_rhs)
            {
                // Cannot assign from RHS for now
                // TODO: Change this?
                co_return {};
            }

            if (assignment_operators.contains(operator_name))
            {
                allowed_operations.insert(primitive_function_info{
                    .overload = function_overload{ .builtin= true, .call_parameters = call_type{.named_parameters = {{"THIS", make_oref(int_type)}}, .positional_parameters = {int_type}}},
                    .return_type = int_type,

                });
            }


            co_return (allowed_operations);
        }

        else if (typeis< numeric_literal_reference >(parent) && name.starts_with("OPERATOR"))
        {
            std::set< primitive_function_info > allowed_operations;

            allowed_operations.insert(primitive_function_info{
                .overload = function_overload{.builtin= true, .call_parameters = {.named_parameters = {{"THIS", numeric_literal_reference{}}, {"OTHER", numeric_literal_reference{}}}}},
                .return_type = numeric_literal_reference{}
            });
            // TODO: MAYBE: Allow composing any integer operation?
            // allowed_operations.push_back(call_parameter_information{{numeric_literal_reference{}, }});
            co_return (allowed_operations);
        }
        else if (name == "CONSTRUCTOR")
        {
            if (typeis< primitive_type_integer_reference >(parent))
            {
                auto int_type = as< primitive_type_integer_reference >(parent);

                std::set< primitive_function_info > result;
                result.insert(primitive_function_info{
                    .overload = function_overload{.builtin= true, .call_parameters = {.named_parameters = {{"THIS", create_nslot(parent)}, {"OTHER", make_cref(parent)}}}},
                    .return_type = void_type{}
                });
                result.insert(primitive_function_info{
                    .overload = function_overload{.builtin= true, .call_parameters = {.named_parameters = {{"THIS", create_nslot(parent)}, {"OTHER", numeric_literal_reference{}}}}},
                    .return_type = void_type{}
                });
                result.insert(primitive_function_info{
                    .overload = function_overload{.builtin= true, .call_parameters = {.named_parameters = {{"THIS", create_nslot(parent)}}}},
                    .return_type = void_type{}
                });
                co_return (result);
            }

            bool should_autogen_constructor = co_await *c->lk_class_should_autogen_default_constructor(parent);

            if (!class_ent.has_value() || should_autogen_constructor)
            {
                std::set< primitive_function_info > result;
                result.insert(primitive_function_info{
                    .overload = function_overload{.builtin= true, .call_parameters = call_type{.named_parameters = {{"THIS", make_mref(parent)}}}},
                    .return_type = parent
                });
                co_return result;
            }
        }

        else if (name == "DESTRUCTOR")
        {
            std::set< primitive_function_info > result;
            result.insert(primitive_function_info{
                .overload = function_overload{.builtin= true, .call_parameters = {.named_parameters = {{"THIS", make_mref(parent)}}}},
                .return_type = void_type{}
            });
            co_return (result);
        }
    }

    co_return {};
}