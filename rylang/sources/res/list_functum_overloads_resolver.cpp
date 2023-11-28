//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/list_functum_overloads_resolver.hpp"

#include "rylang/operators.hpp"

rpnx::resolver_coroutine< rylang::compiler, std::optional< std::set< rylang::call_parameter_information > > > rylang::list_functum_overloads_resolver::co_process(compiler* c, qualified_symbol_reference functum)
{
    bool defaulted = false;
    bool constructor = false;
    std::optional< qualified_symbol_reference > parent_opt;

    std::string name = to_string(functum);

    if (typeis< subdotentity_reference >(functum))
    {
        auto parent = as< subdotentity_reference >(functum).parent;
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
                co_return std::nullopt;
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
            constructor = true;
            // TODO: Allow disabling default constructor
            defaulted = true;

            if (typeis< primitive_type_integer_reference >(parent))
            {
                auto int_type = as< primitive_type_integer_reference >(parent);

                std::set< call_parameter_information > result;
                result.insert({{make_mref(parent), parent}});
                result.insert({{make_mref(parent)}});
                co_return (result);
            }
        }

        else if (name == "DESTRUCTOR")
        {
            // TODO: Allow disabling default constructor
            defaulted = true;
        }
    }

    auto exists = co_await *c->lk_entity_canonical_chain_exists(functum);

    if (!exists && !defaulted)
    {
        co_return (std::nullopt);
    }
    std::set< call_parameter_information > result;

    if (exists)
    {

        auto functum_ast = co_await *c->lk_entity_ast_from_canonical_chain(functum);

        if (!std::holds_alternative< functum_entity_ast >(functum_ast.m_specialization.get()))
        {
            co_return(std::nullopt);
        }

        functum_entity_ast const& functum_e = std::get< functum_entity_ast >(functum_ast.m_specialization.get());

        for (function_ast const& f : functum_e.m_function_overloads)
        {
            call_parameter_information info;
            for (auto const& arg : f.args)
            {
                contextual_type_reference ctx_type;
                ctx_type.context = functum;
                ctx_type.type = arg.type;
                auto type = co_await * c->lk_canonical_type_from_contextual_type(ctx_type);
                info.argument_types.push_back(type);
            }
            result.insert(info);
        }
    }
    if (defaulted)
    {
        call_parameter_information default_consturctor_parameters;
        default_consturctor_parameters.argument_types.push_back(make_mref(parent_opt.value()));

        result.insert(default_consturctor_parameters);
    }
    co_return(result);
}
