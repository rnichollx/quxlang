//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/res/list_builtin_functum_overloads_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/operators.hpp"

rpnx::resolver_coroutine< rylang::compiler, std::set< rylang::call_parameter_information > > rylang::list_builtin_functum_overloads_resolver::co_process(compiler* c, type_symbol functum)
{
    std::optional< type_symbol > parent_opt;
    std::string name = to_string(functum);
    if (typeis< subdotentity_reference >(functum))
    {
        auto parent = as< subdotentity_reference >(functum).parent;

        std::optional< ast2_declaration > decl;

        auto parent_class_exists = co_await *c->lk_entity_canonical_chain_exists(parent);
        if (parent_class_exists)
        {
            decl = co_await *c->lk_entity_ast_from_canonical_chain(parent);

            if (!typeis< ast2_class_declaration >(decl.value()))
            {
                decl = std::nullopt;
            }
        }

        parent_opt = parent;
        auto name = as< subdotentity_reference >(functum).subdotentity_name;

        if (typeis< primitive_type_integer_reference >(parent) && name.starts_with("OPERATOR"))
        {
            auto int_type = as< primitive_type_integer_reference >(parent);
            std::set< call_parameter_information > allowed_operations;

            std::string operator_name = name.substr(8);
            bool is_rhs = false;
            if (operator_name.ends_with("RHS"))
            {
                operator_name = operator_name.substr(0, operator_name.size() - 3);
                allowed_operations.insert(call_parameter_information{{int_type, int_type}});
                is_rhs = true;
            }

            if (assignment_operators.contains(operator_name) && is_rhs)
            {
                // Cannot assign from RHS for now
                // TODO: Change this?
                co_return {};
            }

            if (assignment_operators.contains(operator_name))
            {
                allowed_operations.insert(call_parameter_information{{make_oref(int_type), int_type}});
                co_return allowed_operations;
            }

            allowed_operations.insert(call_parameter_information{{int_type, int_type}});
            // allowed_operations.push_back(call_parameter_information{{int_type, numeric_literal_reference{}}});
            co_return (allowed_operations);
        }

        else if (typeis< numeric_literal_reference >(parent) && name.starts_with("OPERATOR"))
        {
            std::set< call_parameter_information > allowed_operations;

            allowed_operations.insert(call_parameter_information{{numeric_literal_reference{}, numeric_literal_reference{}}});
            // TODO: MAYBE: Allow composing any integer operation?
            // allowed_operations.push_back(call_parameter_information{{numeric_literal_reference{}, }});
            co_return (allowed_operations);
        }
        else if (name == "CONSTRUCTOR")
        {
            if (typeis< primitive_type_integer_reference >(parent))
            {
                auto int_type = as< primitive_type_integer_reference >(parent);

                std::set< call_parameter_information > result;
                result.insert({{make_mref(parent), parent}});
                result.insert({{make_mref(parent)}});
                co_return (result);
            }

            else if (class_ent_ptr == nullptr || !class_ent_ptr->m_keywords.contains("NO_DEFAULT_CONSTRUCTOR"))
            {
                std::set< call_parameter_information > result;
                result.insert({{make_mref(parent)}});
                co_return (result);
            }
        }

        else if (name == "DESTRUCTOR")
        {
            std::set< call_parameter_information > result;
            result.insert({{make_mref(parent)}});
            co_return (result);
        }
    }

    co_return {};
}
